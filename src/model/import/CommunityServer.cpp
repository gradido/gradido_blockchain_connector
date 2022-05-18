#include "CommunityServer.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "ConnectionManager.h"
#include "../table/BaseTable.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace import {
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

			select << "SELECT id, hex(public_key) from state_users", into(stateUsersList);

			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load state users", "state_users", "");
			}
			speedLog.information("loaded %u state users from db in: %s", (unsigned)stateUsersList.size(), loadStateUsersTime.string());

			// sort state users into map
			Profiler sortStateUsersTime;
			std::for_each(stateUsersList.begin(), stateUsersList.end(), [&](const StateUserTuple& stateUser) {
				mStateUserIdPublicKey.insert({ stateUser.get<0>(), stateUser.get<1>() });
			});

			speedLog.information("sorted state users in map: %s", sortStateUsersTime.string());
			speedLog.information("[CommunityServer::loadStateUsers] time: %s", timeUsedAll.string());
		}

		void CommunityServer::loadTransactionsIntoTransactionManager(const std::string& groupAlias)
		{
			Profiler timeUsedAll;
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");
			auto tm = TransactionsManager::getInstance();
			auto mm = MemoryManager::getInstance();

			// load creation transactions
			Profiler loadingCreationsTime;
			typedef Poco::Tuple<uint64_t, std::string, Poco::DateTime, uint64_t, uint64_t, Poco::DateTime> CreationTransactionTuple;
			std::list<CreationTransactionTuple> creationTransactions;

			select << "select t.id, t.memo, FROM_UNIXTIME(t.received), tc.state_user_id, tc.amount, FROM_UNIXTIME(tc.target_date) "
				   << "from transaction_creations as tc "
				   << "JOIN transactions as t "
				   << "ON t.id = tc.transaction_id; ", into(creationTransactions);

			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load creation transactions joined with transactions", "creation_transactions JOIN transactions", "");
			}
			speedLog.information("loaded %u creation transactions from db in: %s", (unsigned)creationTransactions.size(), loadingCreationsTime.string());

			// put creation transactions into TransactionManager
			Profiler putCreationsIntoTransactionManagerTime;
			std::for_each(creationTransactions.begin(), creationTransactions.end(), [&](const CreationTransactionTuple& creationTransaction) {
				auto userPubkey = getUserPubkey(creationTransaction.get<3>(), creationTransaction.get<0>());
				if (!userPubkey) return;

				auto amountString = std::to_string((double)creationTransaction.get<4>() / 10000.0);
				auto creationTransactionObj = TransactionFactory::createTransactionCreation(
					userPubkey,
					amountString,
					groupAlias,
					creationTransaction.get<5>()
				);
				creationTransactionObj->setMemo(creationTransaction.get<1>());
				creationTransactionObj->setCreated(creationTransaction.get<2>());
				tm->pushGradidoTransaction(groupAlias, creationTransaction.get<0>(), std::move(creationTransactionObj));
				mm->releaseMemory(userPubkey);
			});
			creationTransactions.clear();
			speedLog.information("put creation transactions into TransactionManager in: %s", putCreationsIntoTransactionManagerTime.string());

			// load transfer transactions 
			Profiler loadingTransfersTime;
			typedef Poco::Tuple<uint64_t, std::string, Poco::DateTime, uint64_t, uint64_t, uint64_t> TransferTransactionTuple;
			std::list<TransferTransactionTuple> transferTransactions;
			select.reset(dbSession);
			select << "select t.id, t.memo, FROM_UNIXTIME(t.received), tsc.state_user_id, tsc.receiver_user_id, tsc.amount "
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
				
				auto senderUserPubkey = getUserPubkey(transfer.get<3>(), transfer.get<0>());
				if (!senderUserPubkey) return;

				auto recipientUserPubkey = getUserPubkey(transfer.get<4>(), transfer.get<0>());
				if (!recipientUserPubkey) {
					mm->releaseMemory(senderUserPubkey);
					return;
				}
				auto amountString = std::to_string((double)transfer.get<5>() / 10000.0);
				auto transferrTransactionObj = TransactionFactory::createTransactionTransfer(
					senderUserPubkey,
					amountString,
					groupAlias,
					recipientUserPubkey
				);
				transferrTransactionObj->setMemo(transfer.get<1>());
				transferrTransactionObj->setCreated(transfer.get<2>());
				tm->pushGradidoTransaction(groupAlias, transfer.get<0>(), std::move(transferrTransactionObj));

				mm->releaseMemory(senderUserPubkey);
				mm->releaseMemory(recipientUserPubkey);
			});
			transferTransactions.clear();
			speedLog.information("put transfer transactions into TransactionManager in: %s", putTransfersIntoTransactionManagerTime.string());
			speedLog.information("[CommunityServer::loadTransactionsIntoTransactionManager] time: %s", timeUsedAll.string());
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
	}
}
