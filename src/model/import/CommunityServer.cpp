#include "CommunityServer.h"
#include "LoginServer.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "ConnectionManager.h"
#include "../table/BaseTable.h"
#include "../../JSONInterface/JsonTransaction.h"


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
			typedef Poco::Tuple<uint64_t, std::string, Poco::DateTime, uint64_t, uint64_t, Poco::DateTime, std::string> CreationTransactionTuple;
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
				auto userPubkey = getUserPubkey(creationTransaction.get<3>(), creationTransaction.get<0>());
				if (!userPubkey) return;

				if (userKeys) {
					auto senderUserPubkeyHex = *userPubkey->convertToHex().get();
					auto senderPubkeyIt = userKeys->find(senderUserPubkeyHex);
					if (senderPubkeyIt == userKeys->end()) {
						auto keyPair = getReserveKeyPair(senderUserPubkeyHex);
						missingPrivKeys.insert(senderUserPubkeyHex);
						mm->releaseMemory(userPubkey);
						userPubkey = keyPair->getPublicKeyCopy();
					}
				}

				auto amountString = std::to_string((double)creationTransaction.get<4>() / 10000.0);
				auto creationTransactionObj = TransactionFactory::createTransactionCreation(
					userPubkey,
					amountString,
					creationTransaction.get<5>()
				);
				
				//creationTransactionObj->setMemo(creationTransaction.get<1>());
				creationTransactionObj->setCreated(creationTransaction.get<2>());
				creationTransactionObj->updateBodyBytes();
				auto signerPubkeyHex = DataTypeConverter::binToHex(creationTransaction.get<6>());
				if (userKeys) {
					auto signerIt = userKeys->find(signerPubkeyHex);
					if (signerIt == userKeys->end()) {
						missingPrivKeys.insert(signerPubkeyHex);
						// pubkey from admin
						std::string adminPubkey("7fe0a2489bc9f2db54a8f7be6f7d31da7d6d7b2ed2e0d2350b5feec5212589ab");
						adminPubkey.resize(65);
						std::string admin2Pubkey("c6edaa40822a521806b7d86fbdf4c9a0fc8671a9d2e8c432e27e111fc263d51d");
						admin2Pubkey.resize(65);
						if (creationTransaction.get<3>() != 82) {
							signerIt = userKeys->find(adminPubkey);
						}
						else {
							signerIt = userKeys->find(admin2Pubkey);
						}
					}
					
					auto signerPubkey = signerIt->second->getPublicKeyCopy();					

					auto encryptedMemo = JsonTransaction::encryptMemo(
						creationTransaction.get<1>(),
						userPubkey->data(),
						signerIt->second->getPrivateKey()
					);
					creationTransactionObj->setMemo(encryptedMemo).updateBodyBytes();

					auto signature = signerIt->second->sign(*creationTransactionObj->getTransactionBody()->getBodyBytes());
					creationTransactionObj->addSign(signerPubkey, signature);
					if (creationTransaction.get<4>() != 30000000) {
						try {
							creationTransactionObj->validate(gradido::TRANSACTION_VALIDATION_SINGLE);
						}
						catch (GradidoBlockchainException& ex) {
							printf("validation error in transaction %d: %s\n", creationTransaction.get<0>(), ex.getFullString().data());
							throw;
						}
					}
					mm->releaseMemory(signerPubkey);
					mm->releaseMemory(signature);
				}
				tm->pushGradidoTransaction(groupAlias, std::move(creationTransactionObj));
				mm->releaseMemory(userPubkey);
			});
			creationTransactions.clear();
			speedLog.information("put creation transactions into TransactionManager in: %s", putCreationsIntoTransactionManagerTime.string());

			// load transfer transactions 
			Profiler loadingTransfersTime;
			typedef Poco::Tuple<uint64_t, std::string, Poco::DateTime, uint64_t, uint64_t, uint64_t> TransferTransactionTuple;
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
				auto senderUserPubkey = getUserPubkey(transfer.get<3>(), transfer.get<0>());
				if (!senderUserPubkey) return;

				KeyPairEd25519* keyPair = nullptr;
				if (userKeys) {
					auto senderUserPubkeyHex = *senderUserPubkey->convertToHex().get();
					auto signerKeyPairIt = userKeys->find(senderUserPubkeyHex);
					if (signerKeyPairIt == userKeys->end()) {
						keyPair = getReserveKeyPair(senderUserPubkeyHex);
						missingPrivKeys.insert(senderUserPubkeyHex);
						mm->releaseMemory(senderUserPubkey);
						senderUserPubkey = keyPair->getPublicKeyCopy();
					}
					else {
						keyPair = signerKeyPairIt->second.get();
					}
				}

				auto recipientUserPubkey = getUserPubkey(transfer.get<4>(), transfer.get<0>());
				if (!recipientUserPubkey) {
					mm->releaseMemory(senderUserPubkey);
					return;
				}
				auto amountString = std::to_string((double)transfer.get<5>() / 10000.0);
				auto transferrTransactionObj = TransactionFactory::createTransactionTransfer(
					senderUserPubkey,
					amountString,
					"",
					recipientUserPubkey
				);
				//transferrTransactionObj->setMemo(transfer.get<1>());
				transferrTransactionObj->setCreated(transfer.get<2>());
				transferrTransactionObj->updateBodyBytes();
				if (userKeys) {
					auto encryptedMemo = JsonTransaction::encryptMemo(
						transfer.get<1>(),
						recipientUserPubkey->data(),
						keyPair->getPrivateKey()
					);
					transferrTransactionObj->setMemo(encryptedMemo).updateBodyBytes();
					
					auto signature = keyPair->sign(*transferrTransactionObj->getTransactionBody()->getBodyBytes().get());
					transferrTransactionObj->addSign(senderUserPubkey, signature);
					mm->releaseMemory(signature);
					
				}
				tm->pushGradidoTransaction(groupAlias, std::move(transferrTransactionObj));
				mm->releaseMemory(senderUserPubkey);
				mm->releaseMemory(recipientUserPubkey);
			});
			transferTransactions.clear();
			speedLog.information("put transfer transactions into TransactionManager in: %s", putTransfersIntoTransactionManagerTime.string());
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
			auto it = mReserveKeyPairs.find(originalPubkeyHex);
			if (it == mReserveKeyPairs.end()) {
				auto passphrase = Passphrase::generate(&CryptoConfig::g_Mnemonic_WordLists[CryptoConfig::MNEMONIC_BIP0039_SORTED_ORDER]);
				it = mReserveKeyPairs.insert({ originalPubkeyHex, std::move(KeyPairEd25519::create(passphrase)) }).first;
			}
			return it->second.get();
		}
		
	}
}
