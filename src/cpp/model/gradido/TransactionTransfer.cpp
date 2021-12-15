#include "TransactionTransfer.h"
#include "Transaction.h"

#include <sodium.h>

namespace model {
	namespace gradido {

		const std::string TransactionTransfer::mInvalidIndexMessage("invalid index");


		// ********************************************************************************************************************************

		TransactionTransfer::TransactionTransfer(const std::string& memo, const proto::gradido::GradidoTransfer& protoTransfer)
			: TransactionBase(memo), mProtoTransfer(protoTransfer)
		{

		}

		TransactionTransfer::~TransactionTransfer()
		{
			
		}

		int TransactionTransfer::prepare()
		{
			Poco::ScopedLock<Poco::Mutex> _lock(mWorkMutex);
			const static char functionName[] = { "TransactionTransfer::prepare" };

			if (mIsPrepared) return 0;

			proto::gradido::TransferAmount* sender = nullptr;
			std::string* receiver_pubkey = nullptr;
			proto::gradido::LocalTransfer local_transfer;
			if (mProtoTransfer.has_local()) {
				local_transfer = mProtoTransfer.local();
			}
			else if (mProtoTransfer.has_inbound()) {
				local_transfer = mProtoTransfer.inbound().transfer();
			}
			else if (mProtoTransfer.has_outbound()) {
				local_transfer = mProtoTransfer.outbound().transfer();
			}
			sender = local_transfer.mutable_sender();
			receiver_pubkey = local_transfer.mutable_recipiant();
			return prepare(sender, receiver_pubkey);

			return -1;
		}

		int TransactionTransfer::prepare(proto::gradido::TransferAmount* sender, std::string* receiver_pubkey)
		{
			assert(sender && receiver_pubkey);

			char pubkeyHexTemp[65];
			auto sender_pubkey = sender->pubkey();
			auto amount = sender->amount();
			
			mMinSignatureCount = 1;
			auto mm = MemoryManager::getInstance();
			auto pubkey_copy = mm->getFreeMemory(crypto_sign_PUBLICKEYBYTES);
			memcpy(*pubkey_copy, sender_pubkey.data(), crypto_sign_PUBLICKEYBYTES);
			mRequiredSignPublicKeys.push_back(pubkey_copy);

			mIsPrepared = true;
			return 0;
		}

		TransactionValidation TransactionTransfer::validate()
		{
			Poco::ScopedLock<Poco::Mutex> _lock(mWorkMutex);
			static const char function_name[] = "TransactionTransfer::validate";
			/*if (!mProtoTransfer.has_local()) {
				addError(new Error(function_name, "only local currently implemented"));
				return TRANSACTION_VALID_CODE_ERROR;
			}*/
			proto::gradido::TransferAmount* sender = nullptr;
			std::string* receiver_pubkey = nullptr;
			proto::gradido::LocalTransfer local_transfer;
			if (mProtoTransfer.has_local()) {
				local_transfer = mProtoTransfer.local();
			}
			else if (mProtoTransfer.has_inbound()) {
				local_transfer = mProtoTransfer.inbound().transfer();
			}
			else if (mProtoTransfer.has_outbound()) {
				local_transfer = mProtoTransfer.outbound().transfer();
			}

			sender = local_transfer.mutable_sender();
			receiver_pubkey = local_transfer.mutable_recipiant();
			return validate(sender, receiver_pubkey);

			return TRANSACTION_VALID_CODE_ERROR;
		}

		TransactionValidation TransactionTransfer::validate(proto::gradido::TransferAmount* sender, std::string* receiver_pubkey)
		{
			assert(sender && receiver_pubkey);

			static const char function_name[] = "TransactionTransfer::validate";
			auto amount = sender->amount();
			if (0 == amount) {
				addError(new Error(function_name, "amount is empty"));
				return TRANSACTION_VALID_INVALID_AMOUNT;
			}
			else if (amount < 0) {
				addError(new Error(function_name, "negative amount"));
				return TRANSACTION_VALID_INVALID_AMOUNT;
			}
			if (receiver_pubkey->size() != crypto_sign_PUBLICKEYBYTES) {
				addError(new Error(function_name, "invalid size of receiver pubkey"));
				return TRANSACTION_VALID_INVALID_PUBKEY;
			}
			if (sender->pubkey().size() != crypto_sign_PUBLICKEYBYTES) {
				addError(new Error(function_name, "invalid size of sender pubkey"));
				return TRANSACTION_VALID_INVALID_PUBKEY;
			}
			if (0 == memcmp(sender->pubkey().data(), receiver_pubkey->data(), crypto_sign_PUBLICKEYBYTES)) {
				addError(new Error(function_name, "sender and receiver are the same"));
				return TRANSACTION_VALID_INVALID_PUBKEY;
			}
			if (mMemo.size() < 5 || mMemo.size() > 150) {
				addError(new Error(function_name, "memo is not set or not in expected range [5;150]"));
				return TRANSACTION_VALID_INVALID_MEMO;
			}

			return TRANSACTION_VALID_OK;
		}

		std::string TransactionTransfer::getTargetGroupAlias()
		{
			Poco::ScopedLock<Poco::Mutex> _lock(mWorkMutex);
			if (mProtoTransfer.has_local()) {
				return "";
			}
			else if (mProtoTransfer.has_inbound()) {
				auto inbound_transfer = mProtoTransfer.inbound();
				return inbound_transfer.other_group();
			}
			else if (mProtoTransfer.has_outbound()) {
				auto outbound_transfer = mProtoTransfer.outbound();
				return outbound_transfer.other_group();
			}
			return "<unkown>";
		}		
	}
}
