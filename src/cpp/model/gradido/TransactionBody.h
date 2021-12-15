#ifndef GRADIDO_LOGIN_SERVER_MODEL_GRADIDO_TRANSACTION_BASE_H
#define GRADIDO_LOGIN_SERVER_MODEL_GRADIDO_TRANSACTION_BASE_H

#include "GroupMemberUpdate.h"
#include "TransactionCreation.h"
#include "TransactionTransfer.h"

#include "proto/gradido/TransactionBody.pb.h"

#include "../../lib/MultithreadContainer.h"

namespace model {
	namespace gradido {

		enum TransactionType {
			TRANSACTION_NONE,
			TRANSACTION_CREATION,
			TRANSACTION_TRANSFER,
			TRANSACTION_GROUP_MEMBER_UPDATE
		};


		class TransactionBody : public Poco::RefCountedObject, UniLib::lib::MultithreadContainer
		{
		public:
			~TransactionBody();

			//! \brief GroupMemberUpdate Transaction
			static Poco::AutoPtr<TransactionBody> create(
				const std::string& memo,
				const MemoryBin* rootPublicKey,
				proto::gradido::GroupMemberUpdate_MemberUpdateType type,
				const std::string& targetGroupAlias
			);

			// generic transfer creation, inbound, outbound and local
			static Poco::AutoPtr<TransactionBody> create(
				const std::string& memo,
				const MemoryBin* senderPublicKey,
				const MemoryBin* recipiantPublicKey,
				Poco::UInt32 amount,
				TransactionTransferType transferType,
				const std::string groupAlias = "",
				Poco::Timestamp pairedTransactionId = Poco::Timestamp()
			);
			//! \brief GradidoCreation Transaction
			static Poco::AutoPtr<TransactionBody> create(
				const std::string& memo,
				const MemoryBin* recipiantPublicKey,
				Poco::UInt32 amount,
				Poco::DateTime targetDate
			);


			static Poco::AutoPtr<TransactionBody> load(const std::string& protoMessageBin);

			inline TransactionType getType() { Poco::ScopedLock<Poco::Mutex> _lock(mWorkMutex); return mType; }
			static const char* transactionTypeToString(TransactionType type);
			std::string getMemo();
			void setMemo(const std::string& memo);

			bool isCreation() { Poco::ScopedLock<Poco::Mutex> _lock(mWorkMutex); return mType == TRANSACTION_CREATION; }
			bool isTransfer() { Poco::ScopedLock<Poco::Mutex> _lock(mWorkMutex); return mType == TRANSACTION_TRANSFER; }
			bool isGroupMemberUpdate() { Poco::ScopedLock<Poco::Mutex> _lock(mWorkMutex); return mType == TRANSACTION_GROUP_MEMBER_UPDATE; }

			std::string getBodyBytes();
			const proto::gradido::TransactionBody* getBody() { return &mTransactionBody; }

			TransactionCreation* getCreationTransaction();
			TransactionTransfer* getTransferTransaction();
			GroupMemberUpdate* getGroupMemberUpdate();
			TransactionBase* getTransactionBase();

		protected:
			TransactionBody();
			proto::gradido::TransactionBody mTransactionBody;
			TransactionBase* mTransactionSpecific;
			TransactionType mType;
		};
	}
}

#endif //GRADIDO_LOGIN_SERVER_MODEL_GRADIDO_TRANSACTION_BASE_H
