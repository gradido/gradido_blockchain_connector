#include "PrepareApolloTransferTransaction.h"
#include "../ServerConfig.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "JSONInterface/JsonTransaction.h"

namespace task
{
	PrepareApolloTransferTransaction::PrepareApolloTransferTransaction(
		Poco::AutoPtr<model::import::ApolloServer> apolloServer,
		model::import::ApolloServer::TransactionTuple transaction,
		const std::string& groupAlias
	) : CPUTask(ServerConfig::g_WorkerThread), mApolloServer(apolloServer), mTransactionTuple(transaction), mGroupAlias(groupAlias)
	{

	}

	PrepareApolloTransferTransaction::~PrepareApolloTransferTransaction()
	{
		
	}

	int PrepareApolloTransferTransaction::run()
	{
		auto mm = MemoryManager::getInstance();
		auto tm = model::TransactionsManager::getInstance();
		auto senderKeyPair = mApolloServer->getUserKeyPair(mTransactionTuple.get<1>());
		assert(senderKeyPair && "sender key pair is zero");

		auto recipientKeyPair = mApolloServer->getUserKeyPair(mTransactionTuple.get<7>());		
		assert(recipientKeyPair && "recipient key pair is zero");
		
		auto senderPubkeyCopy = senderKeyPair->getPublicKeyCopy();
		auto recipientPubkeyCopy = recipientKeyPair->getPublicKeyCopy();

		auto amountString = std::to_string(mTransactionTuple.get<3>());
		auto transferTransactionObj = TransactionFactory::createTransactionTransfer(
			senderPubkeyCopy,
			amountString,
			"",
			recipientPubkeyCopy
		);

		//creationTransactionObj->setMemo(creationTransaction.get<1>());
		transferTransactionObj->setCreated(mTransactionTuple.get<4>());
		transferTransactionObj->updateBodyBytes();

		auto encryptedMemo = JsonTransaction::encryptMemo(
			mTransactionTuple.get<5>(),
			recipientPubkeyCopy->data(),
			senderKeyPair->getPrivateKey()
		);
		transferTransactionObj->setMemo(encryptedMemo).updateBodyBytes();

		auto signature = senderKeyPair->sign(*transferTransactionObj->getTransactionBody()->getBodyBytes());
		transferTransactionObj->addSign(senderPubkeyCopy, signature);

		try {
			transferTransactionObj->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
		}
		catch (GradidoBlockchainException& ex) {
			printf("validation error in transaction %d: %s\n", mTransactionTuple.get<0>(), ex.getFullString().data());
			throw;
		}

		mm->releaseMemory(signature);

		tm->pushGradidoTransaction(mGroupAlias, std::move(transferTransactionObj));
		mm->releaseMemory(senderPubkeyCopy);
		mm->releaseMemory(recipientPubkeyCopy);
		return 0;
	}

	bool PrepareApolloTransferTransaction::isReady()
	{
		auto senderKeyPair = mApolloServer->getUserKeyPair(mTransactionTuple.get<1>());
		auto recipientKeyPair = mApolloServer->getUserKeyPair(mTransactionTuple.get<7>());

		if((!senderKeyPair || !recipientKeyPair) && !mApolloServer->isAllRecoverKeyPairTasksFinished()) {
			return false;
		}
		return true;
	}
}