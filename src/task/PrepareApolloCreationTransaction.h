#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_APOLLO_CREATION_TRANSACTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_APOLLO_CREATION_TRANSACTION_H

#include "CPUTask.h"
#include "model/import/ApolloServer.h"

namespace task 
{
	class PrepareApolloCreationTransaction : public CPUTask
	{
	public:
		PrepareApolloCreationTransaction(
			Poco::AutoPtr<model::import::ApolloServer> apolloServer,
			model::import::ApolloServer::TransactionTuple transaction,
			const std::string& groupAlias
		);
		~PrepareApolloCreationTransaction();

		const char* getResourceType() const { return "PrepareApolloCreationTransaction"; };
		int run();

		bool isReady();

	protected:
		Poco::AutoPtr<model::import::ApolloServer> mApolloServer;
		model::import::ApolloServer::TransactionTuple mTransactionTuple;
		std::string mGroupAlias;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_APOLLO_CREATION_TRANSACTION_H