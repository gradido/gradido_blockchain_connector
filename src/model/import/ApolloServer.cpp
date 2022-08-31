#include "ApolloServer.h"
#include "../../ConnectionManager.h"
#include "../table/BaseTable.h"
#include "../../task/RecoverLoginKeyPair.h"
#include "../../task/PrepareApolloTransaction.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"

#include "Poco/Data/Session.h"
#include "Poco/Data/MySQL/MySQLException.h"
#include "Poco/Logger.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace import {
		ApolloServer::ApolloServer(const std::string& groupAlias)
			: mGroupAlias(groupAlias)
		{
			auto passphrase = Passphrase::generate(&CryptoConfig::g_Mnemonic_WordLists[CryptoConfig::MNEMONIC_BIP0039_SORTED_ORDER]);
			auto passphraseString = passphrase->getString();
			mFounderKeyPair = std::move(KeyPairEd25519::create(passphrase));
			Poco::Logger::get("errorLog").information("new generarted passphrase for founder key: %s", passphraseString);

			auto mm = MemoryManager::getInstance();
			auto tm = TransactionsManager::getInstance();

			auto founderPubkeyCopy = mFounderKeyPair->getPublicKeyCopy();
			auto registerAddress = TransactionFactory::createRegisterAddress(founderPubkeyCopy, proto::gradido::RegisterAddress_AddressType_HUMAN);
			//int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int millisecond = 0, int microsecond = 0
			registerAddress->setCreated(Poco::DateTime(2019, 10, 8, 10));
			registerAddress->updateBodyBytes();
			auto sign = mFounderKeyPair->sign(*registerAddress->getTransactionBody()->getBodyBytes());
			registerAddress->addSign(founderPubkeyCopy, sign);
			mm->releaseMemory(sign);
			mm->releaseMemory(founderPubkeyCopy);
			tm->pushGradidoTransaction(groupAlias, std::move(registerAddress));
		}

		ApolloServer::~ApolloServer()
		{
			printf("[ApolloServer::~ApolloServer] \n");
		}

		void ApolloServer::loadAll(const std::string& groupAlias)
		{
			if (mLoadState) return;
			mLoadState++;

			Profiler allTime;

			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");

			
			Profiler loadingUsersTime;
			typedef Poco::Tuple<uint64_t, std::string, Poco::DateTime, std::string> UserTuples;
			std::list<UserTuples> usersList;

			select << "SELECT id, lower(hex(public_key)), created, passphrase FROM users WHERE passphrase IS NOT NULL", into(usersList);
			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load users", "users", "");
			}

			speedLog.information("loaded %u users from apollo server table in: %s", (unsigned)usersList.size(), loadingUsersTime.string());

			//std::scoped_lock _lock(mWorkMutex);
			Profiler scheduleRecoverKeyPairTasksTime;
			std::for_each(usersList.begin(), usersList.end(), [&](const UserTuples& user) {
				if (user.get<1>().size() >= 64) {
					auto it = mUserIdPubkeyHex.insert({ user.get<0>(), user.get<1>().substr(0, 64) });
				}

				Poco::AutoPtr <task::RecoverLoginKeyPair> task = new task::RecoverLoginKeyPair(
					user.get<0>(),
					user.get<3>(),
					Poco::AutoPtr<LoginServer>(this, true),
					user.get<2>(),
					groupAlias
				);
				
				task->setOriginalPubkeyHex(user.get<1>());
				task->scheduleTask(task);
				mRecoverKeyPairTasks.push_back(task);
			});
			speedLog.information("time for scheduling %u RecoverLoginKeyPair Tasks: %s", (unsigned)usersList.size(), scheduleRecoverKeyPairTasksTime.string());

			loadTransactions(groupAlias);
			speedLog.information("sum time for ApolloServer::loadAll: %s", allTime.string());

			while (!isAllTransactionTasksFinished()) {
				Poco::Thread::sleep(10);
			}
		}

		const KeyPairEd25519* ApolloServer::getUserKeyPair(uint64_t userId)
		{
			Poco::ScopedLock _lock(mApolloWorkMutex);
			auto pubkeyHexIt = mUserIdPubkeyHex.find(userId);
			if (pubkeyHexIt == mUserIdPubkeyHex.end()) {
				auto keyPair = createReserveKeyPair(mGroupAlias);
				auto pubkeyHex = keyPair->getPublicKeyHex().substr(0, 64);
				std::scoped_lock _lock(mWorkMutex);
				auto userKeyIt = mUserKeys.insert({ pubkeyHex, std::move(keyPair) });
				assert(userKeyIt.second);
				mUserIdPubkeyHex.insert({ userId, pubkeyHex });

				Poco::Logger::get("errorLog").information("generate publickey: %s for user: %u",
					pubkeyHex, (unsigned)userId);
				
				return userKeyIt.first->second.get();
			}
		
			return getOrCreateKeyPair(pubkeyHexIt->second, mGroupAlias);
		}

		bool ApolloServer::isAllTransactionTasksFinished()
		{
			std::scoped_lock _lock(mWorkMutex);
			int erasedCount = 0;
			for (auto it = mTransactionTasks.begin(); it != mTransactionTasks.end(); ++it) {
				if ((*it)->isTaskFinished()) {
					it = mTransactionTasks.erase(it);
					erasedCount++;
					if (it == mTransactionTasks.end()) break;
				}
				else {
					return false;
				}
			}
			if (erasedCount) {
				mLoadState++;
			}
			return true;
		}

		void ApolloServer::cleanTransactions()
		{
			LoginServer::cleanTransactions();
			mUserIdPubkeyHex.clear(); 
		}

		LoginServer::UserTuple ApolloServer::getUserInfos(const std::string& pubkeyHex)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			UserTuple result;

			select << "select id, email, first_name, last_name, created from users where hex(public_key) LIKE ?",
				useRef(pubkeyHex), into(result), now;

			return std::move(result);
		}

		void ApolloServer::loadTransactions(const std::string& groupAlias)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");
			Poco::Logger& errorLog = Poco::Logger::get("errorLog");
			Profiler readAllTransactionsTime;

			// id, user_id, type_id, amount, balance_date, memo, creation_date, linked_user_id
			std::list<TransactionTuple> transactionsList;
			try {
				select << "SELECT id, user_id, type_id, IF(type_id = 2, -amount, amount) as amount, balance_date, memo, creation_date, linked_user_id, previous "
					//<< "FROM transactions_temp "
					<< "FROM transactions_temp "
					<< "where type_id IN (1,2) order by balance_date ASC", into(transactionsList), now;
			}
			catch (Poco::Data::MySQL::StatementException& ex) {
				errorLog.error("Mysql Statement Exception: %s", ex.displayText());
				throw;
			}

			speedLog.information("time for reading: %u transactions from db: %s", (unsigned)transactionsList.size(), readAllTransactionsTime.string());

			Profiler scheduleAllTransactionsTime;
			std::vector<TransactionTuple*> tempTransactions;
			std::map<uint64_t, Poco::DateTime> updateBalanceDates;
			Poco::DateTime lastBalanceDate(2010, 1, 1);
			for (auto it = transactionsList.begin(); it != transactionsList.end(); it++) {
				auto balance_date = it->get<4>();
				if (balance_date > lastBalanceDate) {
					if (tempTransactions.size() > 1) {
						std::sort(tempTransactions.begin(), tempTransactions.end(), [](const TransactionTuple* a, const TransactionTuple* b) {
							return a->get<1>() > b->get<1>();
						});
						//errorLog.information("temp count: %u", (unsigned)tempTransactions.size());
						uint64_t last_user_id = 0;
						int i = 1;
						for (auto tempIt = tempTransactions.begin(); tempIt != tempTransactions.end(); tempIt++) {
							if (!last_user_id) {
								last_user_id = (*tempIt)->get<1>();
							}
							else if (last_user_id == (*tempIt)->get<1>()) {
								auto lastrow = tempIt-1;
								//(*tempIt)->set<4>(balance_date + Poco::Timespan(1, 0));
								updateBalanceDates.insert({ (*tempIt)->get<0>(), Poco::DateTime(lastBalanceDate + Poco::Timespan(i, 0)) });
								errorLog.information("update transaction %u balance_date", (unsigned)(*tempIt)->get<0>());
								if ((*tempIt)->get<8>() != (*lastrow)->get<0>()) {
									errorLog.information(
										"wrong order 1: %u, 2: %u", 
										(unsigned)(*lastrow)->get<0>(), (unsigned)(*tempIt)->get<0>()
									);
								}
								i++;
							}
							else {
								last_user_id = (*tempIt)->get<1>();
							}
						}
					}
					// reset
					tempTransactions.clear();
					lastBalanceDate = balance_date;
				}				
				tempTransactions.push_back(&*it);				
			}
			std::for_each(transactionsList.begin(), transactionsList.end(), [&](const TransactionTuple& transaction) {
				auto type_id = transaction.get<2>();
				auto transactionCopy = transaction;
				auto changeBalanceIt = updateBalanceDates.find(transaction.get<0>());
				if (changeBalanceIt != updateBalanceDates.end()) {
					transactionCopy.set<4>(changeBalanceIt->second);
					errorLog.information("update transaction: %u to balance date: %s (%s)",
						(unsigned)transaction.get<0>(), 
						Poco::DateTimeFormatter::format(changeBalanceIt->second, Poco::DateTimeFormat::SORTABLE_FORMAT),
						Poco::DateTimeFormatter::format(transactionCopy.get<4>(), Poco::DateTimeFormat::SORTABLE_FORMAT)
					);
				}
				task::TaskPtr task;
				if (type_id != 1 && type_id != 2) return;
				
				task = new task::PrepareApolloTransaction(Poco::AutoPtr(this, true), transactionCopy, groupAlias);
				
				if (!task.isNull()) {
					mTransactionTasks.push_back(task);
					task->scheduleTask(task);
					/*while (!task->isReady()) {
						Poco::Thread::sleep(100);
					}
					task->run();
				//	*/
				}
			});
			speedLog.information("time for scheduling %u prepare transactions tasks: %s", (unsigned)transactionsList.size(), scheduleAllTransactionsTime.string());
		}
	}
}