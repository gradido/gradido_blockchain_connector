#include "GradidoNodeRPC.h"
#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"
#include "ServerConfig.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

namespace gradidoNodeRPC {

	void handleGradidoNodeRpcException()
	{
		try {
			throw; // assume it was called from catch clause
		}
		catch (GradidoBlockchainException& ex) {
			Poco::Logger::get("errorLog").error("gradido blockchain exception by call to Gradido Node: %s", ex.getFullString());
			throw GradidoNodeRPCException();
		}
		catch (Poco::Exception& ex) {
			Poco::Logger::get("errorLog").error("Poco exception by call to Gradido Node: %s", ex.displayText());
			throw GradidoNodeRPCException();
		}
		catch (std::exception& ex) {
			std::string what = ex.what();
			Poco::Logger::get("errorLog").error("std::exception by call to Gradido Node: %s", what);
			throw GradidoNodeRPCException();
		}
		
	}

	std::string getAddressBalance(
		const std::string& pubkeyHex,
		const std::string& dateString,
		const std::string& groupAlias,
		uint32_t coinColor /*= 0*/
	) {
		try {	
			Value rpcParams(kObjectType);
			JsonRPCRequest askForAddressBalance(ServerConfig::g_GradidoNodeUri);
			auto alloc = askForAddressBalance.getJsonAllocator();
			rpcParams.AddMember("coinColor", coinColor, alloc);
			rpcParams.AddMember("date", Value(dateString.data(), alloc), alloc);
			rpcParams.AddMember("pubkey", Value(pubkeyHex.data(), alloc), alloc);
			rpcParams.AddMember("groupAlias", Value(groupAlias.data(), alloc), alloc);
			auto result = askForAddressBalance.request("getaddressbalance", rpcParams);
			if (!result.HasMember("result")) {
				throw RapidjsonMissingMemberException("missing result from getaddressbalance", "result", "object");
			}
			if (!result["result"].HasMember("balance")) {
				StringBuffer buffer;
				PrettyWriter<StringBuffer> writer(buffer);
				result.Accept(writer);

				const char* output = buffer.GetString();
				printf("result from Gradido Node: %s\n", output);
				throw RapidjsonMissingMemberException("missing in result from getaddressbalance", "balance", "string");
			}
			return std::move(std::string(result["result"]["balance"].GetString()));
		}
		catch (...) {
			handleGradidoNodeRpcException();
		}
	}
}