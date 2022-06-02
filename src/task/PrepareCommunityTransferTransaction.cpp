#include "PrepareCommunityTransferTransaction.h"
#include "ServerConfig.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "../JSONInterface/JsonTransaction.h"
#include "GradidoNodeRPC.h"

namespace task {
	PrepareCommunityTransferTransaction::PrepareCommunityTransferTransaction(
		model::import::CommunityServer::TransferTransactionTuple transaction,
		Poco::AutoPtr<model::import::CommunityServer> communityServer,
		const std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>>* userKeys,
		const std::string groupAlias
	) : CPUTask(ServerConfig::g_WorkerThread), mTransaction(transaction),
		mCommunityServer(communityServer), mUserKeys(userKeys), mGroupAlias(groupAlias)
	{

	}

	PrepareCommunityTransferTransaction::~PrepareCommunityTransferTransaction()
	{

	}

	int PrepareCommunityTransferTransaction::run()
	{
		auto mm = MemoryManager::getInstance();
		auto tm = model::TransactionsManager::getInstance();
		auto senderUserPubkey = mCommunityServer->getUserPubkey(mTransaction.get<3>(), mTransaction.get<0>());
		std::set<std::string> missingPrivKeys;

		if (!senderUserPubkey) return -1;

		KeyPairEd25519* keyPair = nullptr;
		if (mUserKeys) {
			auto senderUserPubkeyHex = *senderUserPubkey->convertToHex().get();
			auto signerKeyPairIt = mUserKeys->find(senderUserPubkeyHex);
			if (signerKeyPairIt == mUserKeys->end()) {
				keyPair = mCommunityServer->getReserveKeyPair(senderUserPubkeyHex, mGroupAlias);
				missingPrivKeys.insert(senderUserPubkeyHex);
				mm->releaseMemory(senderUserPubkey);
				senderUserPubkey = keyPair->getPublicKeyCopy();
			}
			else {
				keyPair = signerKeyPairIt->second.get();
			}
		}

		auto recipientUserPubkey = mCommunityServer->getUserPubkey(mTransaction.get<4>(), mTransaction.get<0>());
		if (!recipientUserPubkey) {
			mm->releaseMemory(senderUserPubkey);
			return -2;
		}
		auto amountString = std::to_string((double)mTransaction.get<5>() / 10000.0);
		auto transferrTransactionObj = TransactionFactory::createTransactionTransfer(
			senderUserPubkey,
			amountString,
			"",
			recipientUserPubkey
		);
		//transferrTransactionObj->setMemo(transfer.get<1>());
		transferrTransactionObj->setCreated(mTransaction.get<2>());
		transferrTransactionObj->updateBodyBytes();
		if (mUserKeys) {
			auto encryptedMemo = JsonTransaction::encryptMemo(
				mTransaction.get<1>(),
				recipientUserPubkey->data(),
				keyPair->getPrivateKey()
			);
			transferrTransactionObj->setMemo(encryptedMemo).updateBodyBytes();

			auto signature = keyPair->sign(*transferrTransactionObj->getTransactionBody()->getBodyBytes().get());
			transferrTransactionObj->addSign(senderUserPubkey, signature);
			mm->releaseMemory(signature);

			try {
				transferrTransactionObj->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
			}
			catch (GradidoBlockchainException& ex) {
				printf("validation error in transaction %d: %s\n", mTransaction.get<0>(), ex.getFullString().data());
				throw;
			}
		}
		tm->pushGradidoTransaction(mGroupAlias, std::move(transferrTransactionObj));
		mm->releaseMemory(senderUserPubkey);
		mm->releaseMemory(recipientUserPubkey);

		/*std::for_each(missingPrivKeys.begin(), missingPrivKeys.end(), [&](const std::string& pubkeyHex) {
			Poco::Logger::get("errorLog").error("key pair for signing transaction for pubkey: %s replaced", pubkeyHex);
		});*/

		return 0;
	}
}