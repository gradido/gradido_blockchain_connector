#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_COMMUNITY_CREATION_TRANSACTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_COMMUNITY_CREATION_TRANSACTION_H

#include "CPUTask.h"
#include "Poco/AutoPtr.h"
#include "../model/import/CommunityServer.h"

namespace task {
	class PrepareCommunityCreationTransaction : public CPUTask
	{
	public: 
		PrepareCommunityCreationTransaction(
			model::import::CommunityServer::CreationTransactionTuple transaction,
			Poco::AutoPtr<model::import::CommunityServer> communityServer,
			const std::string groupAlias
		);
		~PrepareCommunityCreationTransaction();

		const char* getResourceType() const { return "PrepareCommunityCreationTransaction"; };
		int run();
	protected:
		model::import::CommunityServer::CreationTransactionTuple mTransaction;
		Poco::AutoPtr<model::import::CommunityServer> mCommunityServer;
		std::string mGroupAlias;		
	};
}

#endif 