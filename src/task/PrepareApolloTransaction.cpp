#include "PrepareApolloTransaction.h"
#include "../ServerConfig.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "JSONInterface/JsonTransaction.h"

namespace task
{
	PrepareApolloTransaction::PrepareApolloTransaction(
		Poco::AutoPtr<model::import::ApolloServer> apolloServer,
		model::import::ApolloServer::TransactionTuple transaction,
		const std::string& groupAlias
	) : CPUTask(ServerConfig::g_WorkerThread), mApolloServer(apolloServer), mTransactionTuple(transaction), mGroupAlias(groupAlias)
	{

	}

	PrepareApolloTransaction::~PrepareApolloTransaction()
	{

	}

	int PrepareApolloTransaction::run()
	{
		auto mm = MemoryManager::getInstance();
		auto tm = model::TransactionsManager::getInstance();

		
		auto recipientPubkeyCopy = getRecipientPublicKeyCopy();
		assert(recipientPubkeyCopy);

		auto signerKeyPair = getSignerKeyPair();
		assert(signerKeyPair && "signer key pair is empty");
		MemoryBin* signerPubkey = signerKeyPair->getPublicKeyCopy();
		

		auto amountString = std::to_string(mTransactionTuple.get<3>());
		auto type_id = mTransactionTuple.get<2>();
		static std::unique_ptr<model::gradido::GradidoTransaction> gradidoTransaction;
		// creation
		if (type_id == 1) {
			gradidoTransaction = TransactionFactory::createTransactionCreation(
				recipientPubkeyCopy,
				amountString,
				mTransactionTuple.get<6>()
			);
		}
		// send
		if (type_id == 2) {
			gradidoTransaction = TransactionFactory::createTransactionTransfer(
				signerPubkey,
				amountString,
				"",
				recipientPubkeyCopy
			);
		}
		

		//creationTransactionObj->setMemo(creationTransaction.get<1>());
		gradidoTransaction->setCreated(mTransactionTuple.get<4>());
		gradidoTransaction->updateBodyBytes();
		
		auto encryptedMemo = JsonTransaction::encryptMemo(
			mTransactionTuple.get<5>(),
			recipientPubkeyCopy->data(),
			signerKeyPair->getPrivateKey()
		);
		gradidoTransaction->setMemo(encryptedMemo).updateBodyBytes();

		auto signature = signerKeyPair->sign(*gradidoTransaction->getTransactionBody()->getBodyBytes());
		gradidoTransaction->addSign(signerPubkey, signature);

		try {
			gradidoTransaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
		}
		catch (GradidoBlockchainException& ex) {
			printf("validation error in transaction %d: %s\n", mTransactionTuple.get<0>(), ex.getFullString().data());
			throw;
		}

		mm->releaseMemory(signerPubkey);
		mm->releaseMemory(signature);

		tm->pushGradidoTransaction(mGroupAlias, std::move(gradidoTransaction));
		mm->releaseMemory(recipientPubkeyCopy);
		return 0;
	}

	bool PrepareApolloTransaction::isReady()
	{
		auto userKeyPair = mApolloServer->getUserKeyPair(mTransactionTuple.get<1>());
		if (!userKeyPair && !mApolloServer->isAllRecoverKeyPairTasksFinished()) {
			return false;
		}
		return true;
	}

	const KeyPairEd25519* PrepareApolloTransaction::getSignerKeyPair()
	{
		auto type_id = mTransactionTuple.get<2>();
		// creation
		if (type_id == 1) {
			return mApolloServer->getFounderKeyPair();
		
		}
		// send
		if (type_id == 2) {
			return mApolloServer->getUserKeyPair(mTransactionTuple.get<1>());
		}
	}

	MemoryBin* PrepareApolloTransaction::getRecipientPublicKeyCopy()
	{
		auto type_id = mTransactionTuple.get<2>();
		// creation
		if (type_id == 1) {
			return mApolloServer->getUserKeyPair(mTransactionTuple.get<1>())->getPublicKeyCopy();

		}
		// send
		if (type_id == 2) {
			return mApolloServer->getUserKeyPair(mTransactionTuple.get<7>())->getPublicKeyCopy();
		}
		return nullptr;
	}
}