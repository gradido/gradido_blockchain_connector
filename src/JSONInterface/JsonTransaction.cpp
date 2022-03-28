#include "JsonTransaction.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/http/RequestExceptions.h"
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

void JsonTransaction::signAndSendTransaction(
	std::unique_ptr<model::gradido::GradidoTransaction> localOrOubound,
	std::unique_ptr<model::gradido::GradidoTransaction> inbound/* = nullptr*/
)
{

}