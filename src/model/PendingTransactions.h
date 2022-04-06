#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H

#include "gradido_blockchain/model/protobufWrapper/TransactionBase.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"

#include "Poco/DateTime.h"

#include "rapidjson/document.h"

#define MAX_PENDING_TRANSACTIONS_IN_LIST 100

namespace model {

	enum PendingType
	{
		PENDING_SENDED,
		PENDING_CONFIRMED,
		PENDING_REJECTED
	};

	struct PendingTransaction
	{
		PendingTransaction(const std::string& _iotaMessageId, model::gradido::TransactionType _transactionType);

		std::string iotaMessageId;
		model::gradido::TransactionType transactionType;
		Poco::DateTime created;
		PendingType pendingType;
		std::string errorMessage;
	};

	class PendingTransactions: public MultithreadContainer
	{
	public:
		~PendingTransactions() {};
		static PendingTransactions* getInstance();

		void pushNewTransaction(const std::string& iotaMessageId, model::gradido::TransactionType transactionType);
		void updateTransaction(const std::string& iotaMessageId, bool confirmend, const std::string& errorMessage = "");

		rapidjson::Value listAsJson(rapidjson::Document::AllocatorType& alloc) const;
		static const char* pendingTypeToString(PendingType type);
			
	protected:
		PendingTransactions() {};
		std::list<PendingTransaction> mPendingTransactions;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H