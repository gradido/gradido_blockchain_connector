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
			Profiler readAllTransactionsTime;

			// id, user_id, type_id, amount, balance_date, memo, creation_date, linked_user_id
			std::list<TransactionTuple> transactionsList;
			select << "SELECT id, user_id, type_id, IF(type_id = 2, -amount, amount) as amount, balance_date, memo, creation_date, linked_user_id "
				//<< "FROM transactions_temp "
				<< "FROM transactions "
				<< "where type_id IN (1,2)", into(transactionsList), now;

			speedLog.information("time for reading: %u transactions from db: %s", (unsigned)transactionsList.size(), readAllTransactionsTime.string());

			Profiler scheduleAllTransactionsTime;
			std::for_each(transactionsList.begin(), transactionsList.end(), [&](const TransactionTuple& transaction) {
				auto type_id = transaction.get<2>();
				task::TaskPtr task;
				if (type_id != 1 && type_id != 2) return;
				
				task = new task::PrepareApolloTransaction(Poco::AutoPtr(this, true), transaction, groupAlias);
				
				if (!task.isNull()) {
					mTransactionTasks.push_back(task);
					task->scheduleTask(task);
					/*while (!task->isReady()) {
						Poco::Thread::sleep(100);
					}
					task->run();
					*/
				}
			});
			speedLog.information("time for scheduling %u prepare transactions tasks: %s", (unsigned)transactionsList.size(), scheduleAllTransactionsTime.string());
		}
	}
}