/*!
*
* \author: einhornimmond
*
* \date: 25.10.19
*
* \brief: Creation Transaction
*/
#ifndef GRADIDO_LOGIN_SERVER_MODEL_TRANSACTION_TRANSFER_INCLUDE
#define GRADIDO_LOGIN_SERVER_MODEL_TRANSACTION_TRANSFER_INCLUDE

#pragma warning(disable:4800)

#include "TransactionBase.h"
//#include "Transaction.h"
#include "../proto/gradido/GradidoTransfer.pb.h"


namespace model {
	namespace gradido {

		class Transaction;

		enum TransactionTransferType
		{
			TRANSFER_LOCAL,
			TRANSFER_CROSS_GROUP_INBOUND,
			TRANSFER_CROSS_GROUP_OUTBOUND
		};

		class TransactionTransfer : public TransactionBase
		{
		public:
			TransactionTransfer(const std::string& memo, const proto::gradido::GradidoTransfer& protoTransfer);
			~TransactionTransfer();

			int prepare();
			TransactionValidation validate();

			std::string getTargetGroupAlias();
			// needed for iota index
			std::string getOwnGroupAlias() { return mOwnGroupAlias; }
			bool isInbound() { return mProtoTransfer.has_inbound(); }
			bool isOutbound() { return mProtoTransfer.has_outbound(); }
			bool isCrossGroup() { return mProtoTransfer.has_inbound() || mProtoTransfer.has_outbound(); }


			inline void setOwnGroupAlias(const std::string& ownGroupAlias) { mOwnGroupAlias = ownGroupAlias; }
			inline void setTargetGroupAlias(const std::string& targetGroupAlias) { mTargetGroupAlias = targetGroupAlias; }

		protected:
			const static std::string mInvalidIndexMessage;

			int prepare(proto::gradido::TransferAmount* sender, std::string* receiver_pubkey);
			TransactionValidation validate(proto::gradido::TransferAmount* sender, std::string* receiver_pubkey);

			const proto::gradido::GradidoTransfer& mProtoTransfer;
			
			std::string mOwnGroupAlias;
			std::string mTargetGroupAlias;
		};
	}
}


#endif //GRADIDO_LOGIN_SERVER_MODEL_TRANSACTION_TRANSFER_INCLUDE
