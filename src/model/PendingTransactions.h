#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H

#include "gradido_blockchain/model/protobufWrapper/TransactionBase.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"

#include "Poco/DateTime.h"

#include "rapidjson/document.h"

//! after how many transactions in list, oldest entries will be removed
#define MAX_PENDING_TRANSACTIONS_IN_LIST 100

namespace model {

	/*!	Store pending transaction in memory
		@author einhornimmond 

		- store last MAX_PENDING_TRANSACTIONS_IN_LIST Transactions sended with Blockchain Connector as PendingTransaction in list
		- need also a <a href="https://github.com/gradido/gradido_node">Gradido Node</a> configured to notify GradidoBlockchainConnector after getting new transactions from iota
		- Singleton Class (only one instance exist)

		@startuml{uml_pending_transaction.png} "Pending Transactions"
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
		 
		/*! 
			Container for a Pending Transaction
			- contain iota message id as identifier
			- store State of transaction (sended, confirmed, rejected)
			- contain additional informations from apollo server
		*/
		struct PendingTransaction
		{
			PendingTransaction(
				const std::string& _iotaMessageId, 
				model::gradido::TransactionType _transactionType,
				const std::string& _apolloCreatedDecay,
				Poco::DateTime _apolloDecayStart,
				uint64_t	   _apolloTransactionId
			);

			/*! transaction state enum
				describe if transaction was confirmed or rejected from Gradido Node
			*/
			enum class State: uint8_t
			{
				//! initial type, after sending transaction via iota
				SENDED,
				//! after gradido node confirm that the transaction with this iota message id was valid
				CONFIRMED,
				//! after gradido node has rejected this transaction
				REJECTED
			};

			static const char* StateToString(State type);

			inline const char* getStateString() const {
				return StateToString(state);
			}

			//! message id returned from iota after handing in transaction, used as identifer 
			std::string iotaMessageId;
			//! Type of transaction from <a href="https://gradido.github.io/gradido_blockchain/group__enums.html#ga63424b05ada401e320ed8c4473623a41">model::gradido::TransactionType</a>
			model::gradido::TransactionType transactionType;
			//! Creation date of transaction
			Poco::DateTime created;

			//! describe if transaction was confirmed or rejected from Gradido Node
			State state;
			//! error message from Gradido Node if transaction was rejected
			std::string errorMessage;

			//! apollos decay calculation from last transaction to created date
			std::string apolloCreatedDecay;
			Poco::DateTime apolloDecayStart; 
			//! apollo transaction id, his identifier for this transaction
			uint64_t apolloTransactionId;
		};

		~PendingTransactions() {};
		//! return single instance
		static PendingTransactions* getInstance();

		/*! \brief Push new pending transaction to list, called after transaction was pushed to Iota.

			Remove oldest entries from list if entry count exceed MAX_PENDING_TRANSACTIONS_IN_LIST.
			\param iotaMessageId returned message id form iota
			\param transactionType type of transaction
			\param apolloCreatedDecay apollos decay calculation from last transaction to created date
		 */
		void pushNewTransaction(PendingTransaction pendingTransaction);
		/*! \brief Update state of pending transaction.
		 
		 	Traverse list from end to begin, because the probability is high that the searched transaction is the last or one of the last. <br>
			Should be called from /notify which is called from Gradido Node.
			\param iotaMessageId identifier for transaction
			\param confirmed 
			- true if transaction was validated from Gradido Node
			- false if transaction was rejected from Gradido Node
			\param errorMessage error string if confirmed was false
		*/ 
		void updateTransaction(const std::string& iotaMessageId, bool confirmend, const std::string& errorMessage = "");

		/*! \brief export list of PendingTransactions as json array
			\param alloc allocator from rapidjson::Document, the root node of the json object
		*/
		rapidjson::Value listAsJson(rapidjson::Document::AllocatorType& alloc) const;

		std::list<PendingTransaction>::const_iterator findTransaction(const std::string& iotaMessageId) const;
		inline std::list<PendingTransaction>::const_iterator getPendingTransactionsEnd() const { return mPendingTransactions.end(); }
		bool isConfirmed(const std::string& iotaMessageId) const;
		bool isRejected(const std::string& iotaMessageId) const;
		
		
		/*! \brief Check if apollo calculated the same decay as Gradido Node would.
			
			Ask Gradido Node for balance from last transaction, calculate decay until created and compare
		*/
		bool validateApolloCreationDecay(const model::gradido::GradidoTransaction* gradidoTransaction);
			
	protected:
		
		PendingTransactions() {};
		std::list<PendingTransaction> mPendingTransactions;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_PENDING_TRANSACTIONS_H