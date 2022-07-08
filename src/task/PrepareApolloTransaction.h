#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_APOLLO_TRANSACTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_APOLLO_TRANSACTION_H

#include "CPUTask.h"
#include "model/import/ApolloServer.h"

namespace task {
	class PrepareApolloTransaction : public CPUTask
	{
	public:
		PrepareApolloTransaction(
			Poco::AutoPtr<model::import::ApolloServer> apolloServer,
			model::import::ApolloServer::TransactionTuple transaction,
			const std::string& groupAlias
		);
		~PrepareApolloTransaction();

		const char* getResourceType() const { return "PrepareApolloTransaction"; };
		int run();

		bool isReady();

	protected:
		const KeyPairEd25519* getSignerKeyPair();
		MemoryBin* getRecipientPublicKeyCopy();

		Poco::AutoPtr<model::import::ApolloServer> mApolloServer;
		model::import::ApolloServer::TransactionTuple mTransactionTuple;
		std::string mGroupAlias;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_APOLLO_TRANSACTION_H