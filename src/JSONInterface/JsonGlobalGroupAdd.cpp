#include "JsonGlobalGroupAdd.h"
#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "gradido_blockchain/model/TransactionFactory.h"

#include "ServerConfig.h"

using namespace rapidjson;


/*
	* coinColor: uint32 or hex format
	{
		"created":"2021-01-10 10:00:00",
		"groupName": "Gradido Academy Community",
		"groupAlias":"gdd1",
		"coinColor":"ff0000ff"
	}
	*/

Document JsonGlobalGroupAdd::handle(const Document& params)
{
	auto result = readSharedParameter(params);
	if (result.IsObject()) return result;

	std::string groupName, groupAlias;
	getStringParameter(params, "groupName", groupName);
	getStringParameter(params, "groupAlias", groupAlias);
	auto coinColor = readCoinColor(params);
	try {
		if (coinColor) {
			Value rpcParams(kObjectType);
			JsonRPCRequest askIfCoinColorIsUnique(ServerConfig::g_GradidoNodeUri);
			auto alloc = askIfCoinColorIsUnique.getJsonAllocator();
			rpcParams.AddMember("coinColor", coinColor, alloc);
			auto result = askIfCoinColorIsUnique.request("isGroupUnique", rpcParams);
			auto isUnique = result["isCoinColorUnique"].GetBool();
			if (!isUnique) {
				return stateError("coin color already exist, please choose another or keep empty");
			}
		}
		else {
			JsonRPCRequest getUniqueCoinColor(ServerConfig::g_GradidoNodeUri);
			Value emptyValue(kObjectType);
			auto result = getUniqueCoinColor.request("getRandomUniqueCoinColor", emptyValue);
			coinColor = result["coinColor"].GetUint();
		}
	}
	catch (std::exception& ex) {
		Poco::Logger::get("errorLog").error("Error by requesting Gradido Node: %s", std::string(ex.what()));
		return stateError("error by requesting Gradido Node");
	}
	auto globalGroudAdd = TransactionFactory::createGlobalGroupAdd(groupName, groupAlias, coinColor);
	try {
		auto iotaMessageId = signAndSendTransaction(std::move(globalGroudAdd), GROUP_REGISTER_GROUP_ALIAS);
		auto response = stateSuccess();
		auto alloc = response.GetAllocator();
		response.AddMember("iotaMessageId", Value(iotaMessageId.data(), iotaMessageId.size(), alloc), alloc);
		return response;
	}
	catch (GradidoBlockchainException& ex) {
		return handleSignAndSendTransactionExceptions();
	}

}