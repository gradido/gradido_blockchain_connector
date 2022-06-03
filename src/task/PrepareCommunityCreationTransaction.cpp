#include "PrepareCommunityCreationTransaction.h"
#include "ServerConfig.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "../JSONInterface/JsonTransaction.h"
#include "GradidoNodeRPC.h"

namespace task {
	PrepareCommunityCreationTransaction::PrepareCommunityCreationTransaction(
		model::import::CommunityServer::CreationTransactionTuple transaction,
		Poco::AutoPtr<model::import::CommunityServer> communityServer,
		const std::string groupAlias
	) : CPUTask(ServerConfig::g_WorkerThread), 
		mTransaction(transaction), mCommunityServer(communityServer), 
		mGroupAlias(groupAlias)
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
		auto userKeyPair = mCommunityServer->getOrCreateKeyPair(mTransaction.get<3>(), mGroupAlias);
		assert(userKeyPair);
		auto userPubkeyCopy = userKeyPair->getPublicKeyCopy();

		auto amountString = std::to_string((double)mTransaction.get<4>() / 10000.0);
		auto creationTransactionObj = TransactionFactory::createTransactionCreation(
			userPubkeyCopy,
			amountString,
			mTransaction.get<5>()
		);

		//creationTransactionObj->setMemo(creationTransaction.get<1>());
		creationTransactionObj->setCreated(mTransaction.get<2>());
		creationTransactionObj->updateBodyBytes();
		auto signerPubkeyHex = DataTypeConverter::binToHex(mTransaction.get<6>());
		auto signerKeyPair = mCommunityServer->getOrCreateKeyPair(signerPubkeyHex, mGroupAlias);

		auto signerPubkey = signerKeyPair->getPublicKeyCopy();

		auto encryptedMemo = JsonTransaction::encryptMemo(
			mTransaction.get<1>(),
			userPubkeyCopy->data(),
			signerKeyPair->getPrivateKey()
		);
		creationTransactionObj->setMemo(encryptedMemo).updateBodyBytes();

		auto signature = signerKeyPair->sign(*creationTransactionObj->getTransactionBody()->getBodyBytes());
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
	
		tm->pushGradidoTransaction(mGroupAlias, std::move(creationTransactionObj));
		mm->releaseMemory(userPubkeyCopy);
		
		return 0;
	}
}