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
		const std::string groupAlias
	) : CPUTask(ServerConfig::g_WorkerThread), mTransaction(transaction),
		mCommunityServer(communityServer), mGroupAlias(groupAlias)
	{

	}

	PrepareCommunityTransferTransaction::~PrepareCommunityTransferTransaction()
	{

	}

	int PrepareCommunityTransferTransaction::run()
	{
		auto mm = MemoryManager::getInstance();
		auto tm = model::TransactionsManager::getInstance();
		auto senderKeyPair = mCommunityServer->getOrCreateKeyPair(mTransaction.get<3>(), mGroupAlias);
		auto senderUserPubkeyCopy = senderKeyPair->getPublicKeyCopy();

		auto recipientKeyPair = mCommunityServer->getOrCreateKeyPair(mTransaction.get<4>(), mGroupAlias);
		auto recipientUserPubkeyCopy = recipientKeyPair->getPublicKeyCopy();

		auto amountString = std::to_string((double)mTransaction.get<5>() / 10000.0);
		auto transferrTransactionObj = TransactionFactory::createTransactionTransfer(
			senderUserPubkeyCopy,
			amountString,
			"",
			recipientUserPubkeyCopy
		);
		//transferrTransactionObj->setMemo(transfer.get<1>());
		transferrTransactionObj->setCreated(mTransaction.get<2>());
		transferrTransactionObj->updateBodyBytes();
		
		auto encryptedMemo = JsonTransaction::encryptMemo(
			mTransaction.get<1>(),
			recipientKeyPair->getPublicKey(),
			senderKeyPair->getPrivateKey()
		);
		transferrTransactionObj->setMemo(encryptedMemo).updateBodyBytes();

		auto signature = senderKeyPair->sign(*transferrTransactionObj->getTransactionBody()->getBodyBytes().get());
		transferrTransactionObj->addSign(senderUserPubkeyCopy, signature);
		mm->releaseMemory(signature);

		try {
			transferrTransactionObj->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
		}
		catch (GradidoBlockchainException& ex) {
			printf("validation error in transaction %d: %s\n", mTransaction.get<0>(), ex.getFullString().data());
			throw;
		}
		tm->pushGradidoTransaction(mGroupAlias, std::move(transferrTransactionObj));
		mm->releaseMemory(senderUserPubkeyCopy);
		mm->releaseMemory(recipientUserPubkeyCopy);

		return 0;
	}
}