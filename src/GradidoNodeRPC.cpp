#include "GradidoNodeRPC.h"
#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "gradido_blockchain/http/RequestExceptions.h"
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
		catch (RequestResponseErrorException& ex) {
			throw;
		}
		catch (GradidoBlockchainException& ex) {
			Poco::Logger::get("errorLog").error("gradido blockchain exception by call to Gradido Node: %s", ex.getFullString());
			throw GradidoNodeRPCException("gradido blockchain exception", ex.getFullString());
		}
		catch (Poco::Exception& ex) {
			Poco::Logger::get("errorLog").error("Poco exception by call to Gradido Node: %s", ex.displayText());
			throw GradidoNodeRPCException("Poco exception", ex.displayText());
		}
		catch (std::exception& ex) {
			std::string what = ex.what();
			Poco::Logger::get("errorLog").error("std::exception by call to Gradido Node: %s", what);
			throw GradidoNodeRPCException("std exception", what);
		}
		
	}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

	std::string getAddressBalance(
		const std::string& pubkeyHex,
		const std::string& dateString,
		const std::string& groupAlias,
		const std::string& coinGroupId/* = "" */
	) {
		try {	
			Value rpcParams(kObjectType);
			JsonRPCRequest askForAddressBalance(ServerConfig::g_GradidoNodeUri);
			auto alloc = askForAddressBalance.getJsonAllocator();
			rpcParams.AddMember("coinGroupId", Value(coinGroupId.data(), alloc), alloc);
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

	std::string getAddressType(const std::string& pubkeyHex, const std::string& groupAlias)
	{
		try {
			Value rpcParams(kObjectType);
			JsonRPCRequest askForAddressType(ServerConfig::g_GradidoNodeUri);
			auto alloc = askForAddressType.getJsonAllocator();
			rpcParams.AddMember("pubkey", Value(pubkeyHex.data(), alloc), alloc);
			rpcParams.AddMember("groupAlias", Value(groupAlias.data(), alloc), alloc);
			auto result = askForAddressType.request("getaddresstype", rpcParams);
			if (!result.HasMember("result")) {
				throw RapidjsonMissingMemberException("missing result from getaddresstype", "result", "object");
			}
			if (!result["result"].HasMember("addressType")) {
				StringBuffer buffer;
				PrettyWriter<StringBuffer> writer(buffer);
				result.Accept(writer);

				const char* output = buffer.GetString();
				printf("result from Gradido Node: %s\n", output);
				throw RapidjsonMissingMemberException("missing in result from getaddressbalance", "addressType", "enum as string");
			}
			return std::move(std::string(result["result"]["addressType"].GetString()));
		}
		catch (...) {
			handleGradidoNodeRpcException();
		}
	}

	std::vector<uint64_t> getAddressTxids(const std::string& pubkeyHex, const std::string& groupAlias)
	{
		try {
			Value rpcParams(kObjectType);
			JsonRPCRequest askForAddressTxids(ServerConfig::g_GradidoNodeUri);
			auto alloc = askForAddressTxids.getJsonAllocator();
			rpcParams.AddMember("pubkey", Value(pubkeyHex.data(), alloc), alloc);
			rpcParams.AddMember("groupAlias", Value(groupAlias.data(), alloc), alloc);
			auto result = askForAddressTxids.request("getaddresstxids", rpcParams);
			if (!result.HasMember("result")) {
				throw RapidjsonMissingMemberException("missing result from getaddresstxids", "result", "object");
			}
			if (!result["result"].HasMember("transactionNrs")) {
				StringBuffer buffer;
				PrettyWriter<StringBuffer> writer(buffer);
				result.Accept(writer);

				const char* output = buffer.GetString();
				printf("result from Gradido Node: %s\n", output);
				throw RapidjsonMissingMemberException("missing in result from getaddresstxids", "transactionNrs", "array<uint64>");
			}
			std::vector<uint64_t> transactionNrs;
			auto transactionNrsJson = result["result"]["transactionNrs"].GetArray();
			transactionNrs.reserve(transactionNrsJson.Size());
			for (auto it = transactionNrsJson.Begin(); it != transactionNrsJson.End(); it++) {
				transactionNrs.push_back(it->GetUint64());
			}
			return std::move(transactionNrs);
		}
		catch (...) {
			handleGradidoNodeRpcException();
		}
	}

	std::string getCreationSumForMonth(
		const std::string& pubkeyHex,
		int month,
		int year,
		const std::string& startDateString,
		const std::string& groupAlias
	)
	{
		try {
			Value rpcParams(kObjectType);
			JsonRPCRequest askForAddressBalance(ServerConfig::g_GradidoNodeUri);
			auto alloc = askForAddressBalance.getJsonAllocator();
			rpcParams.AddMember("pubkey", Value(pubkeyHex.data(), alloc), alloc);
			rpcParams.AddMember("month", month, alloc);
			rpcParams.AddMember("year", year, alloc);
			rpcParams.AddMember("startSearchDate", Value(startDateString.data(), alloc), alloc);
			rpcParams.AddMember("groupAlias", Value(groupAlias.data(), alloc), alloc);
			auto result = askForAddressBalance.request("getcreationsumformonth", rpcParams);
			if (!result.HasMember("result")) {
				throw RapidjsonMissingMemberException("missing result from getcreationsumformonth", "result", "object");
			}
			if (!result["result"].HasMember("sum")) {
				StringBuffer buffer;
				PrettyWriter<StringBuffer> writer(buffer);
				result.Accept(writer);

				const char* output = buffer.GetString();
				printf("result from Gradido Node: %s\n", output);
				throw RapidjsonMissingMemberException("missing in result from getcreationsumformonth", "sum", "balance as string");
			}

			std::string sumString = result["result"]["sum"].GetString();
			return std::move(sumString);
		}
		catch (...) {
			handleGradidoNodeRpcException();
		}
	}

	void putTransaction(const std::string& base64Transaction, uint64_t transactionNr, const std::string& groupAlias)
	{
		try {
			Value rpcParams(kObjectType);
			JsonRPCRequest askForAddressBalance(ServerConfig::g_GradidoNodeUri);
			auto alloc = askForAddressBalance.getJsonAllocator();
			rpcParams.AddMember("transaction", Value(base64Transaction.data(), alloc), alloc);
			rpcParams.AddMember("transactionNr", transactionNr, alloc);
			rpcParams.AddMember("groupAlias", Value(groupAlias.data(), alloc), alloc);
			auto result = askForAddressBalance.request("puttransaction", rpcParams);
			if (!result.HasMember("result")) {
				throw RapidjsonMissingMemberException("missing result from puttransaction", "result", "object");
			}
		}
		catch (...) {
			handleGradidoNodeRpcException();
		}
	}

#pragma GCC diagnostic pop

	// Exceptions
	GradidoNodeRPCException::GradidoNodeRPCException(const char* what, const std::string& details) noexcept 
		: GradidoBlockchainException(what), mDetails(details)
	{
	}
	std::string GradidoNodeRPCException::getFullString() const
	{ 
		std::string result = what();
		if (mDetails.size()) {
			result += ", details: " + mDetails;
		}		
		return std::move(result);
	}
	
}