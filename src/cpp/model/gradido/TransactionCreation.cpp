#include "TransactionCreation.h"

#include "lib/DataTypeConverter.h"
#include "Poco/DateTimeFormatter.h"

#include <sodium.h>


namespace model {
	namespace gradido {

		TransactionCreation::TransactionCreation(const std::string& memo, const proto::gradido::GradidoCreation& protoCreation)
			: TransactionBase(memo), mProtoCreation(protoCreation)
		{
			memset(mReceiverPublicHex, 0, 65);
		}

		TransactionCreation::~TransactionCreation()
		{

		}

		int TransactionCreation::prepare()
		{
			if (mIsPrepared) return 0;

			const static char functionName[] = { "TransactionCreation::prepare" };
			if (!mProtoCreation.has_recipiant()) {
				addError(new Error(functionName, "hasn't receiver amount"));
				return -1;
			}
			auto receiver_amount = mProtoCreation.recipiant();

			auto receiverPublic = receiver_amount.pubkey();
			if (receiverPublic.size() != crypto_sign_PUBLICKEYBYTES) {
				addError(new ParamError(functionName, "receiver public invalid: ", receiverPublic.size()));
				return -2;
			}

			sodium_bin2hex(mReceiverPublicHex, 65, (const unsigned char*)receiverPublic.data(), receiverPublic.size());
			
			//
			mMinSignatureCount = 1;
			auto mm = MemoryManager::getInstance();
			auto pubkey_copy = mm->getFreeMemory(crypto_sign_PUBLICKEYBYTES);
			memcpy(*pubkey_copy, receiverPublic.data(), crypto_sign_PUBLICKEYBYTES);
			mForbiddenSignPublicKeys.push_back(pubkey_copy);

			mIsPrepared = true;
			return 0;
		}

		std::string TransactionCreation::getTargetDateString()
		{
			// proto format is seconds, poco timestamp format is microseconds
			Poco::Timestamp pocoStamp(mProtoCreation.target_date().seconds() * 1000 * 1000);
			//Poco::DateTime(pocoStamp);
			return Poco::DateTimeFormatter::format(pocoStamp, "%d. %b %y");
		}

		TransactionValidation TransactionCreation::validate()
		{
			static const char function_name[] = "TransactionCreation::validate";
			auto target_date = Poco::DateTime(DataTypeConverter::convertFromProtoTimestampSeconds(mProtoCreation.target_date()));
			auto now = Poco::DateTime();
			auto mm = MemoryManager::getInstance();
			//  2021-09-01 02:00:00 | 2021-12-04 01:22:14
			if (target_date.year() == now.year()) 
			{
				if (target_date.month() + 2 < now.month()) {
					addError(new Error(function_name, "year is the same, target date month is more than 2 month in past"));
					return TRANSACTION_VALID_INVALID_TARGET_DATE;
				}
				if (target_date.month() > now.month()) {
					addError(new Error(function_name, "year is the same, target date month is in future"));
					return TRANSACTION_VALID_INVALID_TARGET_DATE;
				}
			}
			else if(target_date.year() > now.year()) 
			{
				addError(new Error(function_name, "target date year is in future"));
				return TRANSACTION_VALID_INVALID_TARGET_DATE;
			}
			else if(target_date.year() +1 < now.year())
			{
				addError(new Error(function_name, "target date year is in past"));
				return TRANSACTION_VALID_INVALID_TARGET_DATE;
			}
			else 
			{
				// target_date.year +1 == now.year
				if (target_date.month() + 2 < now.month() + 12) {
					addError(new Error(function_name, "target date is more than 2 month in past"));
					return TRANSACTION_VALID_INVALID_TARGET_DATE;
				}
			}
			auto amount = mProtoCreation.recipiant().amount();
			if (amount > 1000 * 10000) {
				addError(new Error(function_name, "creation amount to high, max 1000 per month"));
				return TRANSACTION_VALID_CREATION_OUT_OF_BORDER;
			}
			if (amount < 10000) {
				addError(new Error(function_name, "creation amount to low, min 1 GDD"));
				return TRANSACTION_VALID_CREATION_OUT_OF_BORDER;
			}
			
			if (mProtoCreation.recipiant().pubkey().size() != crypto_sign_PUBLICKEYBYTES) {
				addError(new Error(function_name, "recipiant pubkey has invalid size"));
				return TRANSACTION_VALID_INVALID_PUBKEY;
			}			
			auto empty = mm->getFreeMemory(crypto_sign_PUBLICKEYBYTES);
			memset(*empty, 0, crypto_sign_PUBLICKEYBYTES);
			if (0 == memcmp(mProtoCreation.recipiant().pubkey().data(), *empty, crypto_sign_PUBLICKEYBYTES)) {
				addError(new Error(function_name, "recipiant pubkey is zero"));
				return TRANSACTION_VALID_INVALID_PUBKEY;
			}
			
			return TRANSACTION_VALID_OK;
		}


	}
}

