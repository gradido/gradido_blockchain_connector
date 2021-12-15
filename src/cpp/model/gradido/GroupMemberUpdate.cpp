#include "GroupMemberUpdate.h"
#include "../../lib/JsonRequest.h"
#include "lib/DataTypeConverter.h"
#include <sodium.h>


namespace model {
	namespace gradido {
		GroupMemberUpdate::GroupMemberUpdate(const proto::gradido::GroupMemberUpdate &protoGroupMemberUpdate)
			: TransactionBase(""), mProtoMemberUpdate(protoGroupMemberUpdate)
		{

		}

		GroupMemberUpdate::~GroupMemberUpdate()
		{

		}

		int GroupMemberUpdate::prepare()
		{
			auto target_group = mProtoMemberUpdate.target_group();
			auto mm = MemoryManager::getInstance();

			if (mIsPrepared) return 0;

			if (mProtoMemberUpdate.user_pubkey().size() != crypto_sign_PUBLICKEYBYTES) {
				return -1;
			}

			auto pubkey_copy = mm->getFreeMemory(crypto_sign_PUBLICKEYBYTES);
			memcpy(*pubkey_copy, mProtoMemberUpdate.user_pubkey().data(), crypto_sign_PUBLICKEYBYTES);
			mRequiredSignPublicKeys.push_back(pubkey_copy);

			mMinSignatureCount = 1;
			mIsPrepared = true;
			return 0;
		}

		TransactionValidation GroupMemberUpdate::validate()
		{
			const static char functionName[] = { "GroupMemberUpdate::validate" };
			if (mProtoMemberUpdate.user_pubkey().size() != crypto_sign_PUBLICKEYBYTES) {
				addError(new Error(functionName, "pubkey not set or wrong size"));
				return TRANSACTION_VALID_INVALID_PUBKEY;
			}

			if (mProtoMemberUpdate.member_update_type() != proto::gradido::GroupMemberUpdate::ADD_USER) {
				addError(new Error(functionName, "user move not implemented yet!"));
				return TRANSACTION_VALID_CODE_ERROR;
			}
			auto target_group = mProtoMemberUpdate.target_group();
			if (!isValidGroupAlias(target_group)) {
				addError(new Error(functionName, "target group isn't valid group alias string "));
				return TRANSACTION_VALID_INVALID_GROUP_ALIAS;
			}

			return TRANSACTION_VALID_OK;
		}

		std::string GroupMemberUpdate::getPublicKeyHex()
		{
			auto user_pubkey = mProtoMemberUpdate.user_pubkey();
			return DataTypeConverter::binToHex((const unsigned char*)user_pubkey.data(), user_pubkey.size());
		}
	}
}