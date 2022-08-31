#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_COMPARE_APOLLO_WITH_GRADIDO_NODE_
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_COMPARE_APOLLO_WITH_GRADIDO_NODE_

#include "CPUTask.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionBody.h"

namespace task {

	/*!
	 @author einhornimmond
	 @date 17.07.2022

	 compare transactions from one user from apollo db backup with transactions from this user from gradido node
	*/
	class CompareApolloWithGradidoNode : public CPUTask
	{
	public:
		CompareApolloWithGradidoNode(uint64_t userId, const std::string& userPublicKeyHex, const std::string& groupAlias);
		~CompareApolloWithGradidoNode();
		const char* getResourceType() const { return "CompareApolloWithGradidoNode"; };
		int run();

		inline const std::string& getErrorString() const { return mErrorString; }
		inline bool isIdentical() const { return mIdentical; }
		inline uint64_t getUserId() const { return mUserId; }

	protected:
		const char* getNodeTransactionType(const model::gradido::TransactionBody* body);
		const char* getDBTransactionType(uint8_t typeId);
		//! \param decimalName: only used for error message
		//! \return false if decimal aren't identical
		bool        isDecimalIdentical(mpfr_ptr value1, mpfr_ptr value2, const char* decimalName = "balance");

		void logSuccess(int transactionCount);

		uint64_t mUserId;
		std::string mUserPublicKeyHex;
		std::string mGroupAlias;
		bool mIdentical;
		std::string mErrorString;
	};

	

}

#endif 