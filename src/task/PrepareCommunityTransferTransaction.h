#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_COMMUNITY_TRANSFER_TRANSACTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_COMMUNITY_TRANSFER_TRANSACTION_H

#include "CPUTask.h"
#include "Poco/AutoPtr.h"
#include "../model/import/CommunityServer.h"

namespace task {

	class PrepareCommunityTransferTransaction : public CPUTask
	{
	public:
		PrepareCommunityTransferTransaction(
			model::import::CommunityServer::TransferTransactionTuple transaction,
			Poco::AutoPtr<model::import::CommunityServer> communityServer,
			const std::string groupAlias
		);
		~PrepareCommunityTransferTransaction();

		const char* getResourceType() const { return "PrepareCommunityTransferTransaction"; };
		int run();
	protected:
		model::import::CommunityServer::TransferTransactionTuple mTransaction;
		Poco::AutoPtr<model::import::CommunityServer> mCommunityServer;
		std::string mGroupAlias;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_PREPARE_COMMUNITY_TRANSFER_TRANSACTION_H