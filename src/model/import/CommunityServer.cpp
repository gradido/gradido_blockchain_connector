#include "CommunityServer.h"
#include "LoginServer.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "ConnectionManager.h"
#include "../table/BaseTable.h"
#include "../../JSONInterface/JsonTransaction.h"
#include "../../task/PrepareCommunityCreationTransaction.h"
#include "../../task/PrepareCommunityTransferTransaction.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace import {

		CommunityServer::StateBalance::StateBalance(const StateBalanceTuple& tuple)
			: recordDate(tuple.get<1>()), amountGddCent(tuple.get<2>())
		{

		}

		CommunityServer::StateUserTransaction::StateUserTransaction(const StateUserTransactionTuple& tuple)
			: userId(tuple.get<0>()), transactionId(tuple.get<1>()), balanceGddCent(tuple.get<2>()), balanceDate(tuple.get<3>())
		{

		}

		CommunityServer::CommunityServer()
		{

		}

		CommunityServer::~CommunityServer()
		{

		}

		void CommunityServer::loadStateUsers()
		{
			Profiler timeUsedAll;
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");

			// load state users from db
			Profiler loadStateUsersTime;
			typedef Poco::Tuple<uint64_t, std::string> StateUserTuple;
			std::list<StateUserTuple> stateUsersList;

			select << "SELECT id, LOWER(HEX(public_key)) from state_users", into(stateUsersList);

			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load state users", "state_users", "");
			}
			speedLog.information("loaded %u state users from db in: %s", (unsigned)stateUsersList.size(), loadStateUsersTime.string());

			// sort state users into map
			Profiler sortStateUsersTime;
			std::for_each(stateUsersList.begin(), stateUsersList.end(), [&](const StateUserTuple& stateUser) {
				mStateUserIdPublicKey.insert({ stateUser.get<0>(), stateUser.get<1>() });
				mPublicKeyStateUserId.insert({ stateUser.get<1>(), stateUser.get<0>() });
			});

			speedLog.information("sorted state users in map: %s", sortStateUsersTime.string());
			speedLog.information("[CommunityServer::loadStateUsers] time: %s", timeUsedAll.string());
		}

		void CommunityServer::loadTransactionsIntoTransactionManager(const std::string& groupAlias, const std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>>* userKeys)
		{
			Profiler timeUsedAll;
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");
			Poco::Logger& errorLog = Poco::Logger::get("errorLog");
			std::set<std::string> missingPrivKeys;
			auto tm = TransactionsManager::getInstance();
			auto mm = MemoryManager::getInstance();

			// load creation transactions
			Profiler loadingCreationsTime;
			
			std::list<CreationTransactionTuple> creationTransactions;
			// add timestamp typ: https://stackoverflow.com/questions/37531690/support-for-mysql-timestamp-in-the-poco-c-libraries
			select << "select t.id, t.memo, t.received, tc.state_user_id, tc.amount, tc.target_date, ts.pubkey "
				<< "from transaction_creations as tc "
				<< "JOIN transactions as t "
				<< "ON t.id = tc.transaction_id "
				<< "LEFT JOIN transaction_signatures as ts "
				<< "ON t.id = ts.transaction_id;", into(creationTransactions);

			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load creation transactions joined with transactions", "creation_transactions JOIN transactions", "");
			}
			speedLog.information("loaded %u creation transactions from db in: %s", (unsigned)creationTransactions.size(), loadingCreationsTime.string());

			// put creation transactions into TransactionManager
			Profiler putCreationsIntoTransactionManagerTime;
			std::for_each(creationTransactions.begin(), creationTransactions.end(), [&](const CreationTransactionTuple& creationTransaction) {
				task::TaskPtr task = new task::PrepareCommunityCreationTransaction(
					creationTransaction,
					Poco::AutoPtr<CommunityServer>(this, true),
					userKeys, groupAlias
				);
				task->scheduleTask(task);
				mPreparingTransactions.push_back(task);
			});
			creationTransactions.clear();
			speedLog.information("start creation transactions tasks in: %s", putCreationsIntoTransactionManagerTime.string());

			// load transfer transactions 
			Profiler loadingTransfersTime;
			std::list<TransferTransactionTuple> transferTransactions;
			select.reset(dbSession);
			select << "select t.id, t.memo, t.received, tsc.state_user_id, tsc.receiver_user_id, tsc.amount "
				<< "from transaction_send_coins as tsc "
				<< "JOIN transactions as t "
				<< "ON t.id = tsc.transaction_id", into(transferTransactions);

			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load transfer transactions joined with transactions", "send_coins_transactions JOIN transactions", "");
			}
			speedLog.information("loaded %u transfer transactions from db in: %s", (unsigned)transferTransactions.size(), loadingTransfersTime.string());

			// put transfer transaction into TransactionsManager
			Profiler putTransfersIntoTransactionManagerTime;
			std::for_each(transferTransactions.begin(), transferTransactions.end(), [&](const TransferTransactionTuple& transfer) {
				task::TaskPtr task = new task::PrepareCommunityTransferTransaction(
					transfer,
					Poco::AutoPtr<CommunityServer>(this, true),
					userKeys, groupAlias
				);
				task->scheduleTask(task);
				mPreparingTransactions.push_back(task);
			});
			transferTransactions.clear();
			speedLog.information("start transfer transactions tasks in: %s", putTransfersIntoTransactionManagerTime.string());
			speedLog.information("[CommunityServer::loadTransactionsIntoTransactionManager] time: %s", timeUsedAll.string());
			std::for_each(missingPrivKeys.begin(), missingPrivKeys.end(), [&](const std::string& pubkeyHex) {
				errorLog.error("key pair for signing transaction for pubkey: %s replaced", pubkeyHex);
			});
		}

		void CommunityServer::loadStateUserBalances()
		{
			Profiler timeUsed; 
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");

			std::list<StateBalanceTuple> stateBalances;

			select << "select state_user_id, record_date, amount "
				<< "from state_balances ", into(stateBalances);
			
			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load state balances", "state_balances", "");
			}
			std::for_each(stateBalances.begin(), stateBalances.end(), [&](const StateBalanceTuple& stateBalance) {
				mStateBalances.insert({ stateBalance.get<0>(), stateBalance });
			});
			stateBalances.clear();

			std::list<StateUserTransactionTuple> stateUserTransactions;
			select.reset(dbSession);

			select << "select state_user_id, transaction_id, balance, balance_date "
				   << "from state_user_transactions ", into(stateUserTransactions);

			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load state user transactions", "state_user_transactions", "");
			}
			std::for_each(stateUserTransactions.begin(), stateUserTransactions.end(), [&](const StateUserTransactionTuple& stateUserTransaction) {
				auto userId = stateUserTransaction.get<0>();
				auto userMapIt = mStateUserTransactions.find(userId);
				if (userMapIt == mStateUserTransactions.end()) {
					auto result = mStateUserTransactions.insert({ userId, std::map<Poco::UInt64, StateUserTransaction>() });
					if (!result.second) {
						Poco::Logger::get("errorLog").error("error create user map entry in CommunityServer::mStateUserTransactions");
						return;
					}
					userMapIt = result.first;
				}
				auto transactionNr = stateUserTransaction.get<1>();
				userMapIt->second.insert({ transactionNr, stateUserTransaction });
			});

			speedLog.information("[CommunityServer::loadStateUserBalances] time: %s", timeUsed.string());
		}

		void CommunityServer::loadAll(
			const std::string& groupAlias,
			bool shouldLoadStateUserBalances /* = false */,
			Poco::AutoPtr<LoginServer> loginServer /*= nullptr*/
			//const std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>>* userKeys/* = nullptr*/
		)
		{
			if (mLoadState) return;
			mLoadState++;
			loadStateUsers();
			if (shouldLoadStateUserBalances) {
				loadStateUserBalances();
			}
			Profiler waitOnRecoverKeys;
			while (!loginServer->isAllRecoverKeyPairTasksFinished()) {
				Poco::Thread::sleep(10);
			}
			Poco::Logger::get("speedLog").information("wait for recover keys: %s", waitOnRecoverKeys.string());
			loadTransactionsIntoTransactionManager(groupAlias, &loginServer->getUserKeys());
			mLoadState++;
		}

		std::string CommunityServer::getUserPubkeyHex(Poco::UInt64 userId)
		{
			auto userIt = mStateUserIdPublicKey.find(userId);
			if (userIt == mStateUserIdPublicKey.end()) {
				throw UserIdNotFoundException("[CommunityServer::getUserPubkeyHex]", userId);
			}
			return userIt->second;
		}

		KeyPairEd25519* CommunityServer::findReserveKeyPair(const unsigned char* pubkey)
		{
			for (auto it = mReserveKeyPairs.begin(); it != mReserveKeyPairs.end(); it++)
			{
				if (it->second->isTheSame(pubkey)) {
					return it->second.get();
				}
			}
			return nullptr;
		}

		bool CommunityServer::isAllTransactionTasksFinished()
		{
			std::scoped_lock _lock(mWorkMutex);
			int erasedCount = 0;
			for (auto it = mPreparingTransactions.begin(); it != mPreparingTransactions.end(); ++it) {
				if ((*it)->isTaskFinished()) {
					it = mPreparingTransactions.erase(it);
					erasedCount++;
					if (it == mPreparingTransactions.end()) break;
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

		MemoryBin* CommunityServer::getUserPubkey(uint64_t userId, uint64_t transactionId)
		{
			auto userIt = mStateUserIdPublicKey.find(userId);
			if (userIt == mStateUserIdPublicKey.end()) {
				Poco::Logger::get("errorLog").error(
					"error, sender user: %d for transaction: %d not found",
					(unsigned)userId,
					transactionId
				);
				return nullptr;
			}
			return DataTypeConverter::hexToBin(userIt->second);
		}

		KeyPairEd25519* CommunityServer::getReserveKeyPair(const std::string& originalPubkeyHex)
		{
			std::scoped_lock _lock(mWorkMutex);
			auto it = mReserveKeyPairs.find(originalPubkeyHex);
			if (it == mReserveKeyPairs.end()) {
				auto passphrase = Passphrase::generate(&CryptoConfig::g_Mnemonic_WordLists[CryptoConfig::MNEMONIC_BIP0039_SORTED_ORDER]);
				it = mReserveKeyPairs.insert({ originalPubkeyHex, std::move(KeyPairEd25519::create(passphrase)) }).first;
			}
			return it->second.get();
		}
		
	}
}
