#include "PrepareCommunityCreationTransaction.h"
#include "ServerConfig.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "../JSONInterface/JsonTransaction.h"

namespace task {
	PrepareCommunityCreationTransaction::PrepareCommunityCreationTransaction(
		model::import::CommunityServer::CreationTransactionTuple transaction,
		Poco::AutoPtr<model::import::CommunityServer> communityServer,
		const std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>>* userKeys,
		const std::string groupAlias
	) : CPUTask(ServerConfig::g_WorkerThread), 
		mTransaction(transaction), mCommunityServer(communityServer), 
		mUserKeys(userKeys), mGroupAlias(groupAlias)
	{
#ifdef _UNI_LIB_DEBUG
		setName(std::to_string(transaction.get<0>()).data());
#endif
	}

	PrepareCommunityCreationTransaction::~PrepareCommunityCreationTransaction()
	{

	}

	int PrepareCommunityCreationTransaction::run()
	{
		auto mm = MemoryManager::getInstance();
		auto tm = model::TransactionsManager::getInstance();
		auto userPubkey = mCommunityServer->getUserPubkey(mTransaction.get<3>(), mTransaction.get<0>());
		std::set<std::string> missingPrivKeys;
		if (!userPubkey) {
			return -1;
		}

		if (mUserKeys) {
			auto senderUserPubkeyHex = *userPubkey->convertToHex().get();
			auto senderPubkeyIt = mUserKeys->find(senderUserPubkeyHex);
			if (senderPubkeyIt == mUserKeys->end()) {
				auto keyPair = mCommunityServer->getReserveKeyPair(senderUserPubkeyHex);
				missingPrivKeys.insert(senderUserPubkeyHex);
				mm->releaseMemory(userPubkey);
				userPubkey = keyPair->getPublicKeyCopy();
			}
		}

		auto amountString = std::to_string((double)mTransaction.get<4>() / 10000.0);
		auto creationTransactionObj = TransactionFactory::createTransactionCreation(
			userPubkey,
			amountString,
			mTransaction.get<5>()
		);

		//creationTransactionObj->setMemo(creationTransaction.get<1>());
		creationTransactionObj->setCreated(mTransaction.get<2>());
		creationTransactionObj->updateBodyBytes();
		auto signerPubkeyHex = DataTypeConverter::binToHex(mTransaction.get<6>());
		if (mUserKeys) {
			auto signerIt = mUserKeys->find(signerPubkeyHex);
			if (signerIt == mUserKeys->end()) {
				missingPrivKeys.insert(signerPubkeyHex);
				// pubkey from admin
				std::string adminPubkey("7fe0a2489bc9f2db54a8f7be6f7d31da7d6d7b2ed2e0d2350b5feec5212589ab");
				adminPubkey.resize(65);
				std::string admin2Pubkey("c6edaa40822a521806b7d86fbdf4c9a0fc8671a9d2e8c432e27e111fc263d51d");
				admin2Pubkey.resize(65);
				if (mTransaction.get<3>() != 82) {
					signerIt = mUserKeys->find(adminPubkey);
				}
				else {
					signerIt = mUserKeys->find(admin2Pubkey);
				}
			}

			auto signerPubkey = signerIt->second->getPublicKeyCopy();

			auto encryptedMemo = JsonTransaction::encryptMemo(
				mTransaction.get<1>(),
				userPubkey->data(),
				signerIt->second->getPrivateKey()
			);
			creationTransactionObj->setMemo(encryptedMemo).updateBodyBytes();

			auto signature = signerIt->second->sign(*creationTransactionObj->getTransactionBody()->getBodyBytes());
			creationTransactionObj->addSign(signerPubkey, signature);
			if (mTransaction.get<4>() != 30000000) {
				try {
					creationTransactionObj->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
				}
				catch (GradidoBlockchainException& ex) {
					printf("validation error in transaction %d: %s\n", mTransaction.get<0>(), ex.getFullString().data());
					throw;
				}
			}
			mm->releaseMemory(signerPubkey);
			mm->releaseMemory(signature);
		}
		tm->pushGradidoTransaction(mGroupAlias, std::move(creationTransactionObj));
		mm->releaseMemory(userPubkey);
		/*std::for_each(missingPrivKeys.begin(), missingPrivKeys.end(), [&](const std::string& pubkeyHex) {
			Poco::Logger::get("errorLog").error("key pair for signing transaction for pubkey: %s replaced", pubkeyHex);
		});*/

		return 0;
	}
}