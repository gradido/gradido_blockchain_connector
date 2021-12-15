/*!
*
* \author: einhornimmond
*
* \date: 25.10.19
*
* \brief: Creation Transaction
*/
#ifndef GRADIDO_LOGIN_SERVER_MODEL_TRANSACTION_CREATION_INCLUDE
#define GRADIDO_LOGIN_SERVER_MODEL_TRANSACTION_CREATION_INCLUDE

#pragma warning(disable:4800)

#include "TransactionBase.h"
#include "proto/gradido/GradidoCreation.pb.h"

namespace model {
	namespace gradido {

		class TransactionCreation : public TransactionBase
		{
		public:
			TransactionCreation(const std::string& memo, const proto::gradido::GradidoCreation& protoCreation);
			~TransactionCreation();

			int prepare();
			//! TODO: check created sum in the last 3 month if 1.000 per month isn't exceed
			//! maybe ask node server and hope the answer came fast
			TransactionValidation validate();

			inline google::protobuf::int64 getAmount() { return mProtoCreation.recipiant().amount(); }
			inline char* getPublicHex() { return mReceiverPublicHex; }

			inline std::string getAmountString() { return amountToString(getAmount()); }
			std::string getTargetDateString();

		protected:
			const proto::gradido::GradidoCreation& mProtoCreation;
			char mReceiverPublicHex[65];
		};
	}
}


#endif //GRADIDO_LOGIN_SERVER_MODEL_TRANSACTION_CREATION_INCLUDE
