#include "PrepareApolloCreationTransaction.h"
#include "../ServerConfig.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "JSONInterface/JsonTransaction.h"

namespace task
{
	PrepareApolloCreationTransaction::PrepareApolloCreationTransaction(
		Poco::AutoPtr<model::import::ApolloServer> apolloServer,
		model::import::ApolloServer::TransactionTuple transaction,
		const std::string& groupAlias
	) : CPUTask(ServerConfig::g_WorkerThread), mApolloServer(apolloServer), mTransactionTuple(transaction), mGroupAlias(groupAlias)
	{

	}

	PrepareApolloCreationTransaction::~PrepareApolloCreationTransaction()
	{
		
	}

	int PrepareApolloCreationTransaction::run()
	{
		auto mm = MemoryManager::getInstance();
		auto tm = model::TransactionsManager::getInstance();

		auto userKeyPair = mApolloServer->getUserKeyPair(mTransactionTuple.get<1>());
		assert(userKeyPair && "user key pair is empty");

		auto founderKeyPair = mApolloServer->getFounderKeyPair();		
		auto userPubkeyCopy = userKeyPair->getPublicKeyCopy();

		auto amountString = std::to_string(mTransactionTuple.get<3>());
		auto creationTransactionObj = TransactionFactory::createTransactionCreation(
			userPubkeyCopy,
			amountString,
			mTransactionTuple.get<6>()
		);

		//creationTransactionObj->setMemo(creationTransaction.get<1>());
		creationTransactionObj->setCreated(mTransactionTuple.get<4>());
		creationTransactionObj->updateBodyBytes();
		auto signerPubkeyHex = founderKeyPair->getPublicKeyHex();
		auto signerPubkey = founderKeyPair->getPublicKeyCopy();

		auto encryptedMemo = JsonTransaction::encryptMemo(
			mTransactionTuple.get<5>(),
			userPubkeyCopy->data(),
			founderKeyPair->getPrivateKey()
		);
		creationTransactionObj->setMemo(encryptedMemo).updateBodyBytes();

		auto signature = founderKeyPair->sign(*creationTransactionObj->getTransactionBody()->getBodyBytes());
		creationTransactionObj->addSign(signerPubkey, signature);

		try {
			creationTransactionObj->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
		}
		catch (GradidoBlockchainException& ex) {
			printf("validation error in transaction %d: %s\n", mTransactionTuple.get<0>(), ex.getFullString().data());
			throw;
		}

		mm->releaseMemory(signerPubkey);
		mm->releaseMemory(signature);

		tm->pushGradidoTransaction(mGroupAlias, std::move(creationTransactionObj));
		mm->releaseMemory(userPubkeyCopy);
		return 0;
	}

	bool PrepareApolloCreationTransaction::isReady()
	{
		auto userKeyPair = mApolloServer->getUserKeyPair(mTransactionTuple.get<1>());
		if(!userKeyPair && !mApolloServer->isAllRecoverKeyPairTasksFinished()) {
			return false;
		}
		return true;
	}
}