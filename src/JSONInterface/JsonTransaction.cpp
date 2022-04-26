#include "JsonTransaction.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/http/IotaRequestExceptions.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

#include "SessionManager.h"
#include "ServerConfig.h"
#include "model/PendingTransactions.h"
#include "model/table/Group.h"

#include "Poco/DateTimeParser.h"
#include "Poco/Timezone.h"

using namespace rapidjson;

Document JsonTransaction::readSharedParameter(const Document& params)
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
	mApolloTransactionId = 0;
	getUInt64Parameter(params, "apolloTransactionId", mApolloTransactionId);
	try {
		mSession = SessionManager::getInstance()->getSession(getJwtToken(), mClientIp.toString());
	}
	catch (JwtTokenException& ex) {
		Poco::Logger::get("errorLog").warning("jwt token exception: %s", ex.getFullString());
		return stateError("invalid jwt token");
	}
	catch (LoginException& ex) {
		return stateError(ex.what());
	}
	catch (SessionException& ex) {
		Poco::Logger::get("errorLog").warning("jwt token exception: %s", ex.getFullString());
		return stateError(ex.what());
	}
	return Document();
}

uint32_t JsonTransaction::readCoinColor(const Document& params)
{
	auto coinColor = params.FindMember("coinColor");
	if (coinColor == params.MemberEnd()) {
		auto group = model::table::Group::load(mSession->getGroupAlias());
		return group->getCoinColor();
	}
	if (coinColor->value.IsString()) {
		auto mm = MemoryManager::getInstance();
		std::string coinColorString = coinColor->value.GetString();
		if (!coinColorString.size()) return 0;
		MemoryBin* coinColorBin = nullptr;
		// TODO: Check seems to be not working correctly
		if (coinColorString.substr(0, 2) == "0x") {
			coinColorBin = DataTypeConverter::hexToBin(coinColorString.substr(2));
		}
		else {
			coinColorBin = DataTypeConverter::hexToBin(coinColorString);
		}
		if (!coinColorBin || coinColorBin->size() != sizeof(uint32_t)) {
			if (coinColorBin) mm->releaseMemory(coinColorBin);
			throw HandleRequestException("coinColor isn't a valid hex string");
		}
		uint32_t result;
		memcpy(&result, *coinColorBin, sizeof(uint32_t));
		mm->releaseMemory(coinColorBin);
		return result;
	}
	else if (coinColor->value.IsUint()) {
		return coinColor->value.GetUint();
	}
	throw HandleRequestException("coinColor has unknown type");
}

std::string JsonTransaction::signAndSendTransaction(std::unique_ptr<model::gradido::GradidoTransaction> transaction, const std::string& groupAlias)
{
	// TODO: encrypt and decrypt memo
	transaction->setMemo(mMemo).setCreated(mCreated).setApolloTransactionId(mApolloTransactionId).updateBodyBytes();
	auto transactionBody = transaction->getTransactionBody();
	
	if (!mSession->signTransaction(transaction.get())) {
		throw Ed25519SignException("cannot sign transaction", mSession->getPublicKey(), *transactionBody->getBodyBytes().get());
	}
	try {
		transaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
	}
	catch (model::gradido::TransactionValidationInvalidSignatureException& ex) {
		printf("invalid signature exception: %s\n", ex.getFullString().data());
		throw;
	}
	
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
	auto iotaMessageId = ServerConfig::g_IotaRequestHandler->sendMessage(DataTypeConverter::binToHex(index), *hex_message);
	model::PendingTransactions::getInstance()->pushNewTransaction(iotaMessageId, transaction->getTransactionBody()->getTransactionType());
	return std::move(iotaMessageId);
}

rapidjson::Document JsonTransaction::handleSignAndSendTransactionExceptions()
{
	try {
		throw; // assume it was called from catch clause
	}
	catch (model::gradido::TransactionValidationInvalidSignatureException& ex) {
		Poco::Logger::get("errorLog").error("invalid signature exception: %s", ex.getFullString());
		return stateError("Internal Server Error");
	}
	catch (model::gradido::TransactionValidationInvalidInputException& ex) {
		return stateError("transaction validation failed", ex.getDetails());
	}
	catch (IotaRequestException& ex) {
		Poco::Logger::get("errorLog").error("error by calling iota: %s", ex.getFullString());
		return stateError("error by calling iota", ex.getDetails());
	}	
	catch (RapidjsonParseErrorException& ex) {
		Poco::Logger::get("errorLog").error("calling iota return invalid json: %s", ex.getFullString());
		return stateError("error by calling iota", ex.getDetails());
	}
	catch (Ed25519SignException& ex) {
		Poco::Logger::get("errorLog").error("error by signing a transaction: %s", ex.getFullString());
		return stateError("Internal Server Error");
	}

	return Document();
}