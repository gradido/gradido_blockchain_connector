#include "Transaction.h"
#include "ServerConfig.h"

#include "lib/DataTypeConverter.h"
#include "lib/Profiler.h"
#include "lib/JsonRequest.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <google/protobuf/util/json_util.h>

#include <inttypes.h>

using namespace rapidjson;

namespace model {
	namespace gradido {
		Transaction::Transaction(Poco::AutoPtr<TransactionBody> body)
			: mTransactionBody(body)
		{
			auto body_bytes = mTransactionBody->getBodyBytes();
			mProtoTransaction.set_body_bytes(body_bytes);
		}

		Transaction::Transaction(const std::string& protoMessageBin)			
		{
			if (!mProtoTransaction.ParseFromString(protoMessageBin)) {
				throw new Poco::Exception("error parse from string");
			}
			mTransactionBody = TransactionBody::load(mProtoTransaction.body_bytes());			
		}

		Transaction::~Transaction()
		{
			
		}

		Poco::AutoPtr<Transaction> Transaction::createGroupMemberUpdate(
			const MemoryBin* rootPublicKey,
			const std::string& groupAlias
		)
		{
			if (!rootPublicKey || !groupAlias.size()) {
				return nullptr;
			}

			auto body = TransactionBody::create(
				"",
				rootPublicKey,
				proto::gradido::GroupMemberUpdate_MemberUpdateType_ADD_USER,
				groupAlias
			);
			Poco::AutoPtr<Transaction> result = new Transaction(body);
			
			return result;
		}

		Poco::AutoPtr<Transaction> Transaction::createCreation(
			const MemoryBin* recipientPubkey,
			Poco::UInt32 amount,
			Poco::DateTime targetDate,
			const std::string& memo
		)
		{
			if (!recipientPubkey || amount <= 0) {
				return nullptr;
			}
			
			auto body = TransactionBody::create(memo, recipientPubkey, amount, targetDate);
			Poco::AutoPtr<Transaction> result = new Transaction(body);

			return result;
		}
		Poco::AutoPtr<Transaction> Transaction::createTransferLocal(
			const MemoryBin* senderPublicKey,
			const MemoryBin* recipientPubkey,
			Poco::UInt32 amount,
			const std::string& memo
		)
		{
			Poco::AutoPtr<Transaction> transaction;
			Poco::AutoPtr<TransactionBody> transaction_body;

			if (!senderPublicKey || !recipientPubkey || !amount) {
				return transaction;
			}

			
			transaction_body = TransactionBody::create(memo, senderPublicKey, recipientPubkey, amount, TRANSFER_LOCAL);
			transaction = new Transaction(transaction_body);
			
			return transaction;
		}

		Poco::AutoPtr<Transaction> Transaction::createTransferCrossGroup(
			const MemoryBin* senderPublicKey,
			const MemoryBin* recipientPubkey,
			const std::string& senderGroupAlias,
			const std::string& recipientGroupAlias,
			Poco::UInt32 amount,
			const std::string& memo)
		{
			Poco::AutoPtr<Transaction> outboundTransaction;
			Poco::AutoPtr<Transaction> inboundTransaction;


			if (!senderPublicKey || !recipientPubkey || !amount || !senderGroupAlias.size() || !recipientGroupAlias.size()) {
				return outboundTransaction;
			}

			Poco::Timestamp now;

			for (int i = 0; i < 2; i++) {
				TransactionTransferType type = TRANSFER_CROSS_GROUP_INBOUND;
				std::string groupAlias = senderGroupAlias;
				if (1 == i) {
					type = TRANSFER_CROSS_GROUP_OUTBOUND;
					groupAlias = recipientGroupAlias;
				}
				Poco::AutoPtr<TransactionBody> body = TransactionBody::create(memo, senderPublicKey, recipientPubkey, amount, type, groupAlias, now);
				Poco::AutoPtr<Transaction> transaction(new Transaction(body));
				if (0 == i) {
					inboundTransaction = transaction;
					inboundTransaction->getTransactionBody()->getTransferTransaction()->setOwnGroupAlias(recipientGroupAlias);
				}
				else if (1 == i) {
					outboundTransaction = transaction;
					outboundTransaction->getTransactionBody()->getTransferTransaction()->setOwnGroupAlias(senderGroupAlias);
				}
			}

			
			return outboundTransaction;
		}
		
		TransactionValidation Transaction::validate()
		{
			auto sig_map = mProtoTransaction.sig_map();

			// check if all signatures belong to current body bytes
			auto body_bytes = mProtoTransaction.body_bytes();
			for (auto it = sig_map.sigpair().begin(); it != sig_map.sigpair().end(); it++)
			{
				if (crypto_sign_verify_detached(
						(const unsigned char*)it->signature().data(),
						(const unsigned char*)body_bytes.data(),
						body_bytes.size(),
						(const unsigned char*)it->pubkey().data()
				) != 0) {
					return TRANSACTION_VALID_INVALID_SIGN;
				}
			}

			auto transaction_base = mTransactionBody->getTransactionBase();
			auto result = transaction_base->checkRequiredSignatures(&sig_map);

			if (result == TRANSACTION_VALID_OK) {
				return transaction_base->validate();
			}
			return result;

		}
		bool Transaction::hasSigned(const MemoryBin* userPublicKey)
		{
			static const char* function_name = "Transaction::hasSigned";
			auto sig_pairs = mProtoTransaction.sig_map().sigpair();
			
			if (crypto_sign_PUBLICKEYBYTES != userPublicKey->size()) {
				addError(new ParamError(function_name, "user public key has invalid size: ", userPublicKey->size()));
				return false;
			}
			for (auto it = sig_pairs.begin(); it != sig_pairs.end(); it++)
			{
				if (it->pubkey().size() != crypto_sign_PUBLICKEYBYTES) {
					addError(new ParamError(function_name, "signed public key has invalid length: ", it->pubkey().size()));
					return false;
				}
				if (memcmp(userPublicKey->data(), it->pubkey().data(), userPublicKey->size()) == 0) {
					return true;
				}
			}
			return false;
		}

		//! return true if user must sign it and hasn't yet
		bool Transaction::mustSign(const MemoryBin* userPublicKey)
		{
			if (hasSigned(userPublicKey)) return false;
			
			auto transaction_base = mTransactionBody->getTransactionBase();
			// inbound transaction will be auto signed when outbound transaction will be signed
			if (mTransactionBody->isTransfer() && mTransactionBody->getTransferTransaction()->isInbound()) {
				return false;
			}
			return transaction_base->isPublicKeyRequired(userPublicKey->data());
		}

		//! return true if user can sign transaction and hasn't yet
		bool Transaction::canSign(const MemoryBin* userPublicKey)
		{
			if (hasSigned(userPublicKey)) return false;
			
			auto transaction_base = mTransactionBody->getTransactionBase();
			// inbound transaction will be auto signed when outbound transaction will be signed
			if (mTransactionBody->isTransfer() && mTransactionBody->getTransferTransaction()->isInbound()) {
				return false;
			}
			return !transaction_base->isPublicKeyForbidden(userPublicKey->data());
		}

		bool Transaction::needSomeoneToSign(const MemoryBin* userPublicKey)
		{
			if (!canSign(userPublicKey)) return false;
			auto transaction_base = mTransactionBody->getTransactionBase();
			if (transaction_base->isPublicKeyRequired(userPublicKey->data())) {
				return false;
			}
			if (transaction_base->getMinSignatureCount() > getSignCount()) {
				return true;
			}
			return false;

		}

		int Transaction::runSendTransaction(const std::string& groupAlias)
		{
			static const char* function_name = "Transaction::runSendTransaction";
			auto result = validate();
			if (TRANSACTION_VALID_OK != result) {
				if (TRANSACTION_VALID_MISSING_SIGN == result || TRANSACTION_VALID_CODE_ERROR == result
					|| TRANSACTION_VALID_MISSING_PARAM == result || TRANSACTION_VALID_INVALID_PUBKEY == result
					|| TRANSACTION_VALID_INVALID_SIGN == result) {
					addError(new ParamError(function_name, "code error", TransactionValidationToString(result)));
					//sendErrorsAsEmail();

				}
				else if (mTransactionBody->isGroupMemberUpdate()) {
					addError(new ParamError(function_name, "validation return: ", TransactionValidationToString(result)));
					//sendErrorsAsEmail();
				}
				else
				{

					std::string error_name;
					std::string error_description;
					switch (result) {
					case TRANSACTION_VALID_FORBIDDEN_SIGN:
						error_name = "Signature Error";
						error_description = "Invalid signature!";
						break;
					case TRANSACTION_VALID_INVALID_TARGET_DATE:
						error_name = "Creation Error";
						error_description = "Invalid target date! No future and only 3 month in the past.";
						break;
					case TRANSACTION_VALID_CREATION_OUT_OF_BORDER:
						error_name = "Creation Error";
						error_description = "Maximal 1000 GDD per month allowed!";
						break;
					case TRANSACTION_VALID_INVALID_AMOUNT:
						error_name = "Transfer Error";
						error_description = "Invalid GDD amount! Amount must be greater than 0 and maximal your balance.";
						break;
					case TRANSACTION_VALID_INVALID_GROUP_ALIAS:
						error_name = "Group Error";
						error_description = "Invalid Group Alias! I didn't know group, please check alias and try again.";
						break;
					default:
						error_name = "Unknown Error";
						error_description = "Unknown";
						addError(new ParamError(function_name, "unknown error", TransactionValidationToString(result)));
						//sendErrorsAsEmail();
					}
					addError(new ParamError(function_name, error_name, error_description));
				}
				return -1;
			}
			else
			{
				std::string raw_message = mProtoTransaction.SerializeAsString();
				if (raw_message == "") {
					addError(new Error(function_name, "error serializing final transaction"));
					return -6;
				}
				auto hex_message = DataTypeConverter::binToHex(raw_message);
				if (hex_message == "") {
					addError(new Error("SendTransactionTask::run", "error convert final transaction to hex"));
					return -7;
				}
								
				return runSendTransactionIota(hex_message, groupAlias);
			}

		}


		int Transaction::runSendTransactionIota(const std::string& transaction_hex, const std::string& groupAlias)
		{
			
			std::string index = "GRADIDO." + groupAlias;
			std::string message = transaction_hex;
			
			auto message_id = ServerConfig::g_IotaRequestHandler->sendMessage(DataTypeConverter::binToHex(index), message, this);
			return 1;
		}

		std::string Transaction::getTransactionAsJson(bool replaceBase64WithHex/* = false*/)
		{
			static const char* function_name = "Transaction::getTransactionAsJson";
			std::string json_message = "";
			std::string json_message_body = "";
			google::protobuf::util::JsonPrintOptions options;
			options.add_whitespace = true;
			options.always_print_primitive_fields = true;

			auto status = google::protobuf::util::MessageToJsonString(mProtoTransaction, &json_message, options);
			if (!status.ok()) {
				addError(new ParamError(function_name, "error by parsing transaction message to json", status.ToString()));
				return "";
			}

			status = google::protobuf::util::MessageToJsonString(*mTransactionBody->getBody(), &json_message_body, options);
			if (!status.ok()) {
				addError(new ParamError(function_name, "error by parsing transaction body message to json", status.ToString()));
				return "";
			}
			//\"bodyBytes\": \"MigKIC7Sihz14RbYNhVAa8V3FSIhwvd0pWVvZqDnVA91dtcbIgRnZGQx\"
			int startBodyBytes = json_message.find("bodyBytes") + std::string("\"bodyBytes\": \"").size() - 2;
			int endCur = json_message.find_first_of('\"', startBodyBytes + 2) + 1;
			json_message.replace(startBodyBytes, endCur - startBodyBytes, json_message_body);
			//printf("json: %s\n", json_message.data());

			if (replaceBase64WithHex) {
				Document parsed_json;
				parsed_json.Parse(json_message.data(), json_message.size());
				if (parsed_json.HasParseError()) {
					addError(new ParamError(function_name, "error by parsing or printing json", parsed_json.GetParseError()));
					addError(new ParamError(function_name, "parsing error offset ", parsed_json.GetErrorOffset()));
				}
				else {
					if (DataTypeConverter::replaceBase64WithHex(parsed_json, parsed_json.GetAllocator())) {
						StringBuffer buffer;
						Writer<StringBuffer> writer(buffer);
						parsed_json.Accept(writer);

						json_message = std::string(buffer.GetString(), buffer.GetLength());
					}
				}
			}

			return json_message;

		}

		bool Transaction::isTheSameTransaction(Poco::AutoPtr<Transaction> other)
		{
			bool result = false;

			auto other_proto = other->getTransactionBody()->getBody();
			auto other_created = other_proto->created();
			auto own_body_bytes = getTransactionBody()->getBodyBytes();
			auto own_body_updated = new proto::gradido::TransactionBody;
			own_body_updated->ParseFromString(own_body_bytes);
			auto own_created = own_body_updated->mutable_created();
			Poco::Int64 timeDiff = other_created.seconds() - own_created->seconds();
			*own_created = other_created;

			result = own_body_updated->SerializeAsString() == other_proto->SerializeAsString();

			delete own_body_updated;

			// if they are more than 10 seconds between transaction they consider as not the same
			if (abs(timeDiff) > 10) {
				return false;
			}

			return result;
		}

	}
}
