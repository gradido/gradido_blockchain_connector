#include "JsonCreationTransaction.h"

#include "Poco/Timezone.h"
#include "Poco/DateTimeParser.h"

#include "model/table/User.h"

#include "gradido_blockchain/model/TransactionFactory.h"

using namespace rapidjson;


	/*
	{
		"created":"2021-01-10 10:00:00",
		"memo": "AGE September 2021",
		"recipientName":"<apollo user identificator>",
		"amount": "100",
		"coinColor": "ffffffff",
		"targetDate": "2021-09-01 01:00:00",
	}
	and jwtoken with user informations
	*/

Document JsonCreationTransaction::handle(const rapidjson::Document& params)
{
	auto mm = MemoryManager::getInstance();

	auto paramError = readSharedParameter(params);
	if (paramError.IsObject()) { return paramError; }

	std::string recipientName, amount, targetDateString;
	uint64_t apolloTransactionId = 0;
	paramError = getStringParameter(params, "recipientName", recipientName);
	if (paramError.IsObject()) { return paramError;}

	paramError = getStringParameter(params, "amount", amount);
	if (paramError.IsObject()) { return paramError;}

	auto coinColor = readCoinColor(params);

	getUInt64Parameter(params, "apolloTransactionId", apolloTransactionId);

	Poco::DateTime targetDate;
	paramError = getStringParameter(params, "targetDate", targetDateString);
	if (paramError.IsObject()) { return paramError; }
	int timezoneDifferential = Poco::Timezone::tzd();
	try {
		targetDate = Poco::DateTimeParser::parse(targetDateString, timezoneDifferential);
		targetDate.makeLocal(Poco::Timezone::tzd());
	}
	catch (Poco::Exception& ex) {
		return stateError("cannot parse targetDate", ex.what());
	}

	auto recipientUser = model::table::User::load(recipientName, mSession->getGroupId());
	if (!recipientUser) {
		return stateError("unknown recipient user");
	}
	auto publicKeyBin = mm->getMemory(32);
	publicKeyBin->copyFromProtoBytes(recipientUser->getPublicKey());
	
	try {
		auto creation = TransactionFactory::createTransactionCreation(publicKeyBin, amount, coinColor, targetDate);
		creation->setApolloTransactionId(apolloTransactionId);
		mm->releaseMemory(publicKeyBin);
		publicKeyBin = nullptr;

		auto iotaMessageId = signAndSendTransaction(std::move(creation), mSession->getGroupAlias());
		auto response = stateSuccess();
		auto alloc = response.GetAllocator();
		response.AddMember("iotaMessageId", Value(iotaMessageId.data(), iotaMessageId.size(), alloc), alloc);
		return response;
	}
	catch (...) {
		if(publicKeyBin) mm->releaseMemory(publicKeyBin);
		throw;
	}

}