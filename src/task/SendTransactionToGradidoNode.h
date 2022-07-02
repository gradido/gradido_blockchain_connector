#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_SEND_TRANSACTION_TO_GRADIDO_NODE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_SEND_TRANSACTION_TO_GRADIDO_NODE_H

#include "CPUTask.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"
#include "Poco/SharedPtr.h"
#include "model/import/LoginServer.h"

namespace task {
	class SendTransactionToGradidoNode: public CPUTask
	{
	public:
		SendTransactionToGradidoNode(
			std::shared_ptr<model::gradido::GradidoTransaction> transaction,
			uint64_t transactionNr, 
			const std::string& groupAlias,
			MultithreadQueue<double>* runtime,
			Poco::AutoPtr<model::import::LoginServer> loginServer
		);

		const char* getResourceType() const { return "SendTransactionToGradidoNode"; };
		int run();
	protected:
		std::shared_ptr<model::gradido::GradidoTransaction> mTransaction;
		uint64_t mTransactionNr;
		std::string mGroupAlias;
		MultithreadQueue<double>* mRuntime;
		Poco::AutoPtr<model::import::LoginServer> mLoginServer;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_SEND_TRANSACTION_TO_GRADIDO_NODE_H