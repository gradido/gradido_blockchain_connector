#include "JsonListTransactions.h"
#include "SessionManager.h"

#include "model/table/User.h"

#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "rapidjson/prettywriter.h"

using namespace rapidjson;

/*
* {
	"currentPage": 1,
	"pageSize": 25,
	"orderDESC": true,
	"onlyCreations": false
}
*/

Document JsonListTransactions::handle(const Document& params)
{
	mSession = SessionManager::getInstance()->getSession(getJwtToken());
	JsonRPCRequest gradidoNodeRequest(ServerConfig::g_GradidoNodeUri);
	Value jsonRpcParams(kObjectType);
	auto alloc = gradidoNodeRequest.getJsonAllocator();
	for (auto it = params.MemberBegin(); it != params.MemberEnd(); it++) {
		if (strcmp(it->name.GetString(), "currentUser") == 0) {
			
		}
		else {
			jsonRpcParams.AddMember(Value(it->name, alloc), Value(it->value, alloc), alloc);
		}
	}
	//currentUser
	auto pubkeyHex = DataTypeConverter::binToHex(mSession->getPublicKey(), KeyPairEd25519::getPublicKeySize());
	jsonRpcParams.AddMember("currentUser", Value(pubkeyHex.data(), alloc), alloc);
	jsonRpcParams.AddMember("groupAlias", Value(mSession->getGroupAlias().data(), alloc), alloc);		
	auto jsonAnswear = gradidoNodeRequest.request("listtransactions", jsonRpcParams);
	auto resultIt = jsonAnswear.FindMember("result");
	auto transactionListIt = resultIt->value.FindMember("transactionList");
	auto transactionsIt = transactionListIt->value.FindMember("transactions");
	std::vector<std::string> pubkeys;
	for (auto it = transactionsIt->value.Begin(); it != transactionsIt->value.End(); ++it) {
		auto linkedUserIt = it->FindMember("linkedUser");
		auto pubkeyString = std::string(linkedUserIt->value["pubkey"].GetString(), linkedUserIt->value["pubkey"].GetStringLength());
		if (pubkeyString.size()) {
			if (std::find(pubkeys.begin(), pubkeys.end(), pubkeyString) == pubkeys.end()) {
				pubkeys.push_back(pubkeyString);
			}
		}
	}
	// use only one db call to get names from it
	if (pubkeys.size()) {
		mHexPubkeyUserNames = model::table::User::loadUserNamesForPubkeys(pubkeys);
		for (auto it = transactionsIt->value.Begin(); it != transactionsIt->value.End(); ++it) {
			auto linkedUserIt = it->FindMember("linkedUser");
			auto pubkeyString = std::string(linkedUserIt->value["pubkey"].GetString(), linkedUserIt->value["pubkey"].GetStringLength());
			linkedUserIt->value.RemoveMember("pubkey");
			auto userNameIt = mHexPubkeyUserNames.find(pubkeyString);
			if (userNameIt != mHexPubkeyUserNames.end()) {
				linkedUserIt->value.RemoveMember("firstName");
				linkedUserIt->value.AddMember("firstName", Value(userNameIt->second.data(), alloc), alloc);
			}
		}
	}
	
	auto result = stateSuccess();
	auto resultAlloc = result.GetAllocator();
	result.AddMember("transactionList", Value(transactionListIt->value, resultAlloc), resultAlloc);
	return result;
}
