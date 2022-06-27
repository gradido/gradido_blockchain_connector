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
#include "../../task/UpdateCreationTargetDate.h"
#include "Poco/UTF8String.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace import {

		std::string CommunityServer::mTempCreationTableName = "transaction_creations_temp";
		std::string CommunityServer::mTempTransactionsTableName = "transactions_temp";

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
			printf("[CommunityServer::~CommunityServer]\n");
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

		void CommunityServer::updateCreationTargetDate()
		{
			Profiler timeUsedAll;
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement copyTable(dbSession);
			

			// make copy of transaction_creations table
			Profiler copyTableTime;
			copyTable << "DROP TABLE IF EXISTS " << mTempCreationTableName, now;
			copyTable.reset(dbSession);
			copyTable << "CREATE TABLE " << mTempCreationTableName << " LIKE transaction_creations;", now;
			copyTable.reset(dbSession);
			copyTable << "INSERT INTO " << mTempCreationTableName << " SELECT * FROM transaction_creations;", now;

			speedLog.information("copy creations table time: %s", copyTableTime.string());
			copyTableTime.reset();

			copyTable.reset(dbSession);
			copyTable << "DROP TABLE IF EXISTS " << mTempTransactionsTableName, now;
			copyTable.reset(dbSession);
			copyTable << "CREATE TABLE " << mTempTransactionsTableName << " LIKE transactions;", now;
			copyTable.reset(dbSession);
			copyTable << "INSERT INTO " << mTempTransactionsTableName << " SELECT * FROM transactions;", now;
			speedLog.information("copy transactions table time: %s", copyTableTime.string());
			
			// split 3000 GDD Creation into three new transactions
			// id 	transaction_id 	state_user_id 	amount 	    target_date 	     transaction_type_id memo 										 received 	
			// 150 	224 	        275 	        30000000 	2020-03-30 08:59:55	 1 	 				 Aktives Grundeinkommen f�r GL.Dez, Jan, Feb 2020-03-30 08:59:55

			// transactions
			// transaction_type_id memo									received
			// 1				   Aktives Grundeinkommen für GL. Dez   2020-03-30 08:59:55
			// 1				   Aktives Grundeinkommen für GL. Jan   2020-03-30 08:59:55
			// 1				   Aktives Grundeinkommen für GL. Feb   2020-03-30 08:59:55

			// transaction_creations
			// transaction_id state_user_id amount   target_date
			// ?			  275			10000000 2019-12-01 01:00:00
			// ?              275           10000000 2020-01-01 01:00:00
			// ?              275           10000000 2020-02-01 01:00:00

			// 
			// target date, memo
			std::vector<std::pair<Poco::DateTime, std::string>> transactionDivData = {
				{Poco::DateTime(2019, 12, 1, 1, 0, 0), "Aktives Grundeinkommen für GL. Dez"},
				{Poco::DateTime(2020, 1, 1, 1, 0, 0),  "Aktives Grundeinkommen für GL. Jan"},
				{Poco::DateTime(2020, 2, 1, 1, 0, 0),  "Aktives Grundeinkommen für GL. Feb"}
			};
			Profiler splitTransactionTime;
			Poco::Data::Statement insertTransactions(dbSession);
			std::string memo;
			Poco::DateTime received(2020, 3, 30, 8, 59, 55);
			insertTransactions << "INSERT INTO " << mTempTransactionsTableName
				<< "(transaction_type_id, memo, received) VALUES(1, ?, ?)",
				use(memo), use(received);

			Poco::Data::Statement insertCreationTransactions(dbSession);
			int amount = 10000000;
			Poco::DateTime targetDate(2019, 12, 1, 1, 0, 0);
			insertCreationTransactions << "INSERT INTO " << mTempCreationTableName
				<< "(transaction_id, state_user_id, amount, target_date) VALUES(LAST_INSERT_ID(), 275, ?, ?)",
				use(amount), use(targetDate);

			for (auto it = transactionDivData.begin(); it != transactionDivData.end(); it++) {
				targetDate = it->first;
				memo = it->second;
				insertTransactions.execute();
				insertCreationTransactions.execute();
			}
			Poco::Data::Statement removeInvalidTransaction(dbSession);
			removeInvalidTransaction << "delete from " << mTempCreationTableName << " where id = 150", now;
			removeInvalidTransaction.reset(dbSession);
			removeInvalidTransaction << "delete from " << mTempTransactionsTableName << " where id = 224", now;
			speedLog.information("time for split transaction: %s", splitTransactionTime.string());

			// select specific data sets and update them			
			Profiler updateTime;
			typedef Poco::Tuple<std::string, uint8_t, uint16_t> ReplaceSet;
			std::vector< ReplaceSet> replaceSets = {
				{"Dez", 12, 2019},
				{"Jan", 1, 2020},
    			{"Feb", 2, 2020},
				{"M_rz", 3, 2020},
				{"April", 4, 2020}
			};
			std::for_each(replaceSets.begin(), replaceSets.end(), [&](const ReplaceSet& replaceSet) {
				Poco::Data::Statement update(dbSession);
				update << "update " << mTempCreationTableName << " "
					<< "set target_date = DATE_FORMAT(target_date, CONCAT(?, '-', ?, '-', ";
				if (replaceSet.get<0>() == "Feb") {
					update << "IF(DATE_FORMAT(target_date, '%d') <= 28, '%d', 28)";
				}
				else {
					update << "'%d'";
				}
				update << ", ' %H:%i:%s')) "
					<< "where id in( "
					<< "select tc.id "
					<< "from " << mTempCreationTableName << " as tc "
					<< "JOIN " << mTempTransactionsTableName << " as t "
					<< "ON t.id = tc.transaction_id "
					<< "where "
					<< "t.received = tc.target_date and tc.amount <= 10000000 and t.memo LIKE '%" << replaceSet.get<0>() << "%' "
					<< ")", useRef(replaceSet.get<2>()), useRef(replaceSet.get<1>()), now;
				//printf("replaced: %s\n", replaceSet.get<0>().data());
			});
			speedLog.information("time for updating target date from creation transactions: %s", updateTime.string());
			Profiler updateTimeRest;
			Poco::Data::Statement update(dbSession);
			update << "update " << mTempCreationTableName << " "
				<< "set target_date = CAST(DATE_FORMAT(target_date, CONCAT("
				<< "IF(DATE_FORMAT(target_date, '%m') = 1, DATE_FORMAT(target_date, '%Y') - 1, '%Y'),"
				<< "'-',"
				<< "IF(DATE_FORMAT(target_date, '%m') = 1, 12, DATE_FORMAT(target_date, '%m') - 1),"
				<< "'-',"
				<< "IF(DATE_FORMAT(target_date, '%m') = 3, IF(DATE_FORMAT(target_date, '%d') <= 28, '%d', 28), '%d'),"
				<< "' %H:%i:%s')) AS DATETIME)"
				<< "where id in("
				<< "SELECT tc.id "
				<< "from " << mTempCreationTableName << " as tc "
				<< "JOIN " << mTempTransactionsTableName << " as t "
				<< "ON t.id = tc.transaction_id "
				<< "WHERE t.received = tc.target_date AND t.memo NOT LIKE '%Dez%' AND t.memo NOT LIKE '%Jan%' "
				<< " AND t.memo NOT LIKE '%Feb%' AND t.memo NOT LIKE '%M_rz%' AND t.memo NOT LIKE '%April%')";
			
			auto count = update.execute(true);
			speedLog.information("update: %u with memo without month name in: %s", (unsigned)count, updateTimeRest.string());
			//*/
			/*
				for creation transactions where target date is more than 2 month in the past from received
				community server format
				check if decay needs an update
			*/
	/*		Profiler updateTransactionReceivedTime;
			update.reset(dbSession);
			// for received month -1 
			update << "update " << mTempTransactionsTableName << " "
				<< "set received = DATE_FORMAT(received, CONCAT("
				  << "'%Y-', CAST(DATE_FORMAT(received, '%m') AS UNSIGNED) - 1, "
				  << "'-', IF(DATE_FORMAT(received, '%d') <= 28, '%d', 28), ' %H:%i:%s')) "
				<< "where id in( "
					<< "SELECT t.id "
					<< "from " << mTempCreationTableName << " tc "
					<< "JOIN " << mTempTransactionsTableName << " as t ON t.id = tc.transaction_id "
					<< "WHERE "
					<< "(TIMESTAMPDIFF(MONTH, tc.target_date, t.received) > 2 OR "
				    << "CAST(DATE_FORMAT(t.received, '%m') AS SIGNED) - CAST(DATE_FORMAT(tc.target_date, '%m') AS SIGNED) > 2) "
					<< "AND DATE_FORMAT(t.received, '%m') > 1 "
				<< ")";
			count = update.execute(true);

			speedLog.information("time for updating %u transaction received month -1: %s", (unsigned)count, updateTransactionReceivedTime.string());
			updateTransactionReceivedTime.reset();

			update.reset(dbSession);
			// for received year -1, month 12 
			update << "update " << mTempTransactionsTableName << " "
				<< "set received = DATE_FORMAT(received, CONCAT("
				<< "CAST(DATE_FORMAT(received, '%Y') AS UNSIGNED)-1, '-12-%d %H:%i:%s')) "
				<< "where id in( "
				<< "SELECT t.id "
				<< "from " << mTempCreationTableName << " tc "
				<< "JOIN " << mTempTransactionsTableName << " as t ON t.id = tc.transaction_id "
				<< "WHERE "
				<< "(TIMESTAMPDIFF(MONTH, tc.target_date, t.received) > 2 OR "
				<< "CAST(DATE_FORMAT(t.received, '%m') AS SIGNED) - CAST(DATE_FORMAT(tc.target_date, '%m') AS SIGNED) > 2) "
				<< "AND CAST(DATE_FORMAT(t.received, '%m') AS UNSIGNED) = 1 "
				<< ")";
			count = update.execute();
			speedLog.information("time for updating %u transaction received year -1, month 12: %s", (unsigned)count, updateTransactionReceivedTime.string());
			*/
			speedLog.information("[CommunityServer::updateCreationTargetDate] time: %s", timeUsedAll.string());
		}

		void CommunityServer::loadTransactionsIntoTransactionManager(const std::string& groupAlias)
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
				<< "from " << mTempCreationTableName << " as tc "
				<< "JOIN " << mTempTransactionsTableName << " as t "
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
					groupAlias
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
				<< "JOIN " << mTempTransactionsTableName << " as t "
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
					groupAlias
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
			mLoginServer = loginServer;
			loadStateUsers();
			if (shouldLoadStateUserBalances) {
				loadStateUserBalances();
			}
			Profiler waitOnRecoverKeys;
			while (!loginServer->isAllRecoverKeyPairTasksFinished()) {
				Poco::Thread::sleep(10);
			}
			loginServer->cleanTransactions();
			Poco::Logger::get("speedLog").information("wait for recover keys: %s", waitOnRecoverKeys.string());
			updateCreationTargetDate();
			loadTransactionsIntoTransactionManager(groupAlias);
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

		void CommunityServer::cleanTransactions()
		{
			mPreparingTransactions.clear();
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

		const KeyPairEd25519* CommunityServer::getOrCreateKeyPair(uint64_t userId, const std::string& groupAlias)
		{
			auto stateUserIt = mStateUserIdPublicKey.find(userId);
			if (stateUserIt == mStateUserIdPublicKey.end()) {
				Poco::Logger::get("errorLog").error(
					"error, sender user: %d not found",
					(unsigned)userId
				);
				return nullptr;
			}
			else {
				return getOrCreateKeyPair(stateUserIt->second, groupAlias);
			}
			return nullptr;
		}

		const KeyPairEd25519* CommunityServer::getOrCreateKeyPair(const std::string& originalPubkeyHex, const std::string& groupAlias)
		{
			const auto& userKeys = mLoginServer->getUserKeys();

			std::scoped_lock _lock(mWorkMutex);
			auto senderPubkeyIt = userKeys.find(originalPubkeyHex);
			if (senderPubkeyIt == userKeys.end()) {
				return getReserveKeyPair(originalPubkeyHex, groupAlias);
			}
			return senderPubkeyIt->second.get();
		}

		const KeyPairEd25519* CommunityServer::getReserveKeyPair(const std::string& originalPubkeyHex, const std::string& groupAlias)
		{
			std::string pubkexHex = originalPubkeyHex.substr(0, 64);
			auto it = mReserveKeyPairs.find(pubkexHex);
			if (it == mReserveKeyPairs.end()) {
				auto passphrase = Passphrase::generate(&CryptoConfig::g_Mnemonic_WordLists[CryptoConfig::MNEMONIC_BIP0039_SORTED_ORDER]);
				auto keyPair = KeyPairEd25519::create(passphrase);
				auto userPubkey = keyPair->getPublicKeyCopy();
				auto mm = MemoryManager::getInstance();
				auto tm = TransactionsManager::getInstance();
				
				auto registerAddress = TransactionFactory::createRegisterAddress(userPubkey, proto::gradido::RegisterAddress_AddressType_HUMAN);
				//int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int millisecond = 0, int microsecond = 0
				registerAddress->setCreated(Poco::DateTime(2019, 10, 8, 10));
				registerAddress->updateBodyBytes();
				auto sign = keyPair->sign(*registerAddress->getTransactionBody()->getBodyBytes());
				registerAddress->addSign(userPubkey, sign);
				mm->releaseMemory(sign);
				mm->releaseMemory(userPubkey);
				tm->pushGradidoTransaction(groupAlias, std::move(registerAddress));

				it = mReserveKeyPairs.insert({ pubkexHex, std::move(keyPair) }).first;
				Poco::Logger::get("errorLog").information("(%u) replace publickey: %s with: %s",
					(unsigned)mReserveKeyPairs.size(), pubkexHex, it->second->getPublicKeyHex());
			}
			
			return it->second.get();
		}
		
	}
}
