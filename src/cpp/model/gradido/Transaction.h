#ifndef GRADIDO_LOGIN_SERVER_MODEL_GRADIDO_TRANSACTION_H
#define GRADIDO_LOGIN_SERVER_MODEL_GRADIDO_TRANSACTION_H

/*
 * @author: Dario Rekowski
 *
 * @date: 12.10.2020
 *
 * @brief: mainly for signing gradido transaction
*/

#include "proto/gradido/GradidoTransaction.pb.h"
#include "TransactionBody.h"
#include <array>

namespace model {
	namespace gradido {

		class Transaction : public NotificationList, public Poco::RefCountedObject
		{
		public:
			Transaction(Poco::AutoPtr<TransactionBody> body);
			Transaction(const std::string& protoMessageBin);
			virtual ~Transaction();

			// create group add member transaction
			// groupMemberUpdate
			static Poco::AutoPtr<Transaction> createGroupMemberUpdateAdd(
				const MemoryBin* rootPublicKey
			);

			static std::array<Poco::AutoPtr<Transaction>, 2> createGroupMemberUpdateMove(
				MemoryBin* userRootPublic,
				const std::string& currentGroupAlias,
				const std::string& newGroupAlias
			);
			//! \brief transfer
			//! \return
			static Poco::AutoPtr<Transaction> createTransferLocal(
				const MemoryBin* senderPublicKey,
				const MemoryBin* recipientPubkey,
				Poco::Int64 amount,
				const std::string& memo
			);
			static std::array<Poco::AutoPtr<Transaction>, 2> createTransferCrossGroup(
				const MemoryBin* senderPublicKey,
				const MemoryBin* recipientPubkey,
				const std::string& senderGroupAlias,
				const std::string& recipientGroupAlias,
				Poco::Int64 amount,
				const std::string& memo
			);

			//! \brief creation transaction
			static Poco::AutoPtr<Transaction> createCreation(
				const MemoryBin* recipientPubkey,
				Poco::Int64 amount,
				Poco::DateTime targetDate,
				const std::string& memo);

			int getSignCount() { return mProtoTransaction.sig_map().sigpair_size(); }
			TransactionValidation validate();

			//! \brief validate and if valid send transaction via Hedera Consensus Service to node server
			int runSendTransaction(const std::string& groupAlias);

			inline Poco::AutoPtr<TransactionBody> getTransactionBody() { return mTransactionBody; }

			//! \brief get current body bytes from proto transaction and save it into db

			//! \return true if user must sign it and hasn't yet
			bool mustSign(const MemoryBin* userPublicKey);
			//! return true if user can sign transaction and hasn't yet
			bool canSign(const MemoryBin* userPublicKey);

			//! \return true if user has already signed transaction
			bool hasSigned(const MemoryBin* userPublicKey);
			//! \return true if at least one sign is missing and user isn't forbidden and isn't required
			bool needSomeoneToSign(const MemoryBin* userPublicKey);

			std::string getTransactionAsJson(bool replaceBase64WithHex = false);

			// use with care, only available after freshly creating cross group transfer transaction, not after loading from db
			inline Poco::AutoPtr<Transaction> getPairedTransaction() { return mPairedTransaction; }

			bool isTheSameTransaction(Poco::AutoPtr<Transaction> other);

			const proto::gradido::GradidoTransaction& getProtoTransaction() { return mProtoTransaction; }

		protected:

			int runSendTransactionIota(const std::string& transaction_hex, const std::string& groupAlias);

			Poco::AutoPtr<Transaction> mPairedTransaction;

			Poco::AutoPtr<TransactionBody> mTransactionBody;
			proto::gradido::GradidoTransaction mProtoTransaction;
		};

	}
}

#endif //GRADIDO_LOGIN_SERVER_MODEL_GRADIDO_TRANSACTION_H
