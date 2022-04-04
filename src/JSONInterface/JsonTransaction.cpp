#include "JsonTransaction.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

#include "SessionManager.h"

#include "Poco/DateTimeParser.h"
#include "Poco/Timezone.h"

rapidjson::Document JsonTransaction::readSharedParameter(const rapidjson::Document& params)
{
	getStringParameter(params, "memo", mMemo);

	std::string created_string;
	auto paramError = getStringParameter(params, "created", created_string);
	if (paramError.IsObject()) { return paramError; }

	int timezoneDifferential = Poco::Timezone::tzd();
	try {
		mCreated = Poco::DateTimeParser::parse(created_string, timezoneDifferential);
	}
	catch (Poco::Exception& ex) {
		return stateError("cannot parse created", ex.what());
	}
	mSession = SessionManager::getInstance()->getSession(getJwtToken());
	return rapidjson::Document();
}

uint32_t JsonTransaction::readCoinColor(const rapidjson::Document& params)
{
	auto coinColor = params.FindMember("coinColor");
	if (coinColor == params.MemberEnd()) {
		return 0;
	}
	if (coinColor->value.IsString()) {
		std::string coinColorString = coinColor->value.GetString();
		auto coinColorBin = DataTypeConverter::hexToBin(coinColorString);
		if (!coinColorBin || coinColorBin->size() != sizeof(uint32_t)) {
			throw HandleRequestException("coinColor isn't a valid hex string");
		}
		uint32_t result;
		memcpy(&result, *coinColorBin, sizeof(uint32_t));
		return result;
	}
	else if (coinColor->value.IsUint()) {
		return coinColor->value.GetUint();
	}
	throw HandleRequestException("coinColor has unknown type");
}

rapidjson::Document JsonTransaction::signAndSendTransaction(std::unique_ptr<model::gradido::GradidoTransaction> transaction, const std::string& groupAlias)
{
	auto transactionBody = transaction->getTransactionBody();
	if (!mSession->signTransaction(transaction.get())) {
		throw Ed25519SignException("cannot sign transaction", mSession->getPublicKey(), *transactionBody->getBodyBytes().get());
	}
	transaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
	std::string _groupAlias = groupAlias;
	// update target group alias if it is a global group add transaction
	if (transaction->getTransactionBody()->isGlobalGroupAdd()) {
		_groupAlias = GROUP_REGISTER_GROUP_ALIAS;
	}
	if (_groupAlias != GROUP_REGISTER_GROUP_ALIAS && !model::gradido::TransactionBase::isValidGroupAlias(_groupAlias)) {
		throw model::gradido::TransactionValidationInvalidInputException("invalid group alias", "groupAlias", "string, [a-z0-9-]{3,120}");
	}
	// send transaction to iota
	auto raw_message = transaction->getSerialized();

	// finale to hex for iota
	auto hex_message = DataTypeConverter::binToHex(std::move(raw_message));

	std::string index = "GRADIDO." + groupAlias;

	//auto message_id = ServerConfig::g_IotaRequestHandler->sendMessage(DataTypeConverter::binToHex(index), *hex_message);

	auto response = stateSuccess();
	auto alloc = response.GetAllocator();
	//response.AddMember("iotaMessageId", Value(message_id.data(), message_id.size(), alloc), alloc);
	return response;
}