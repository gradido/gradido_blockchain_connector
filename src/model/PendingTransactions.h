#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H

#include "gradido_blockchain/model/protobufWrapper/TransactionBase.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"

#include "Poco/DateTime.h"

#include "rapidjson/document.h"

#define MAX_PENDING_TRANSACTIONS_IN_LIST 100

namespace model {

	/*! Pending Type Enum
	 
	 	Describe if transaction was confirmed or rejected from Gradido Node
	*/
	enum PendingType
	{
		//! initial type, after sending transaction via iota
		PENDING_SENDED,
		//! after gradido node confirm that the transaction with this iota message id was valid
		PENDING_CONFIRMED,
		//! after gradido node has rejected this transaction
		PENDING_REJECTED
	};

	/*! 
		Container for a Pending Transaction
	*/
	struct PendingTransaction
	{
		PendingTransaction(const std::string& _iotaMessageId, model::gradido::TransactionType _transactionType);

		//! message id returned from iota after handing in transaction, used as identifer 
		std::string iotaMessageId;
		//! Type of transaction from <a href="https://gradido.github.io/gradido_blockchain/group__enums.html#ga63424b05ada401e320ed8c4473623a41">model::gradido::TransactionType</a>
		model::gradido::TransactionType transactionType;
		//! Creation date of transaction
		Poco::DateTime created;

		PendingType pendingType;
		std::string errorMessage;
	};

	/*!	Store pending transaction in memory
		@author einhornimmond 

		Store Transactions sended with Blockchain Connector as PendingTransaction in list 
		Need also a <a href="https://github.com/gradido/gradido_node">Gradido Node</a> which was configured to notify this GradidoBlockchainConnector 
		after getting new transactions from iota

		@startuml "Pending Transactions"
		participant GradidoBlockchainConnector as connector
		database iota
		participant GradidoNode as node
		connector -> iota: Gradido Transaction
		activate iota
		activate connector
		iota --> connector: iota message id
		deactivate iota
		connector -> connector: Store PendingTransaction with iota message id in list
		deactivate connector
		node -> iota: ask for new transactions [loop]
		activate iota
		iota --> node: return new transactions
		deactivate iota
		node -> node: validate transaction 
		activate node
		node -> connector: /notify with GradidoBlock or error message
		deactivate node
		activate connector
		connector -> connector: search transaction in list and update it if found
		deactivate connector
		@enduml
	*/

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