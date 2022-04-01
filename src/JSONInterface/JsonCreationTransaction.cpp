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
	paramError = getStringParameter(params, "recipientName", recipientName);
	if (paramError.IsObject()) { return paramError;}

	paramError = getStringParameter(params, "amount", amount);
	if (paramError.IsObject()) { return paramError;}

	auto coinColor = readCoinColor(params);

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

	auto recipientUser = model::table::User::load(recipientName);
	if (!recipientUser) {
		return stateError("unknown recipient user");
	}
	auto publicKeyBin = mm->getMemory(32);
	publicKeyBin->copyFromProtoBytes(recipientUser->getPublicKey());
	
	auto creation = TransactionFactory::createTransactionCreation(publicKeyBin, amount, readCoinColor(params), targetDate);
	mm->releaseMemory(publicKeyBin);

	
}