#include "JsonListTransactions.h"
#include "SessionManager.h"

#include "model/table/User.h"

#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"
#include "rapidjson/prettywriter.h"

#include "Poco/RegularExpression.h"

using namespace rapidjson;

/*
* {
	"currentPage": 1,
	"pageSize": 25,
	"orderDESC": true,
	"onlyCreations": false
}
*/

Poco::RegularExpression g_base64RegExp("^(?:[a-zA-Z0-9+/]{4})*(?:|(?:[a-zA-Z0-9+/]{3}=)|(?:[a-zA-Z0-9+/]{2}==)|(?:[a-zA-Z0-9+/]{1}===))$");

Document JsonListTransactions::handle(const Document& params)
{
	auto mm = MemoryManager::getInstance();
	mSession = SessionManager::getInstance()->getSession(getJwtToken(), mClientIp.toString());
	JsonRPCRequest gradidoNodeRequest(ServerConfig::g_GradidoNodeUri);
	Value jsonRpcParams(kObjectType);
	auto alloc = gradidoNodeRequest.getJsonAllocator();
	for (auto it = params.MemberBegin(); it != params.MemberEnd(); it++) {
		jsonRpcParams.AddMember(Value(it->name, alloc), Value(it->value, alloc), alloc);		
	}
	//currentUser
	auto pubkeyHex = DataTypeConverter::binToHex(mSession->getPublicKey(), KeyPairEd25519::getPublicKeySize());
	jsonRpcParams.AddMember("pubkey", Value(pubkeyHex.data(), alloc), alloc);
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
		for (auto it = transactionsIt->value.Begin(); it != transactionsIt->value.End(); ++it) 
		{
			// replace user
			auto linkedUserIt = it->FindMember("linkedUser");
			auto pubkeyStringHex = std::string(linkedUserIt->value["pubkey"].GetString(), linkedUserIt->value["pubkey"].GetStringLength());
			linkedUserIt->value.RemoveMember("pubkey");
			auto userNameIt = mHexPubkeyUserNames.find(pubkeyStringHex);
			auto typeIdIt = it->FindMember("typeId");
			if (userNameIt != mHexPubkeyUserNames.end()) {
				bool swapFirstName = true;
				if (typeIdIt != it->MemberEnd()) {
					std::string typeId = typeIdIt->value.GetString();
					if (typeId == "CREATE") swapFirstName = false;
				}
				if (swapFirstName) {
					linkedUserIt->value.RemoveMember("firstName");
					linkedUserIt->value.AddMember("firstName", Value(userNameIt->second.data(), alloc), alloc);
				}
			}
			// check memo for encryption
			auto memoIt = it->FindMember("memo");
			if (memoIt != it->MemberEnd() && typeIdIt != it->MemberEnd()) {
				auto memo = memoIt->value.GetString();
				std::string typeId = typeIdIt->value.GetString();
				if (g_base64RegExp.match(memo)) {
					MemoryBin* pubkey = DataTypeConverter::hexToBin(pubkeyStringHex);
					if(!pubkey) continue;
					auto clearMemo = decryptMemo(memo, pubkey->data(), mSession->getKeyPair()->getPrivateKey());
					
					if (clearMemo.size()) {
						it->RemoveMember("memo");
						it->AddMember("memo", Value(clearMemo.data(), alloc), alloc);
					}
					mm->releaseMemory(pubkey);
				}
			}
		}
	}
	
	auto result = stateSuccess();
	auto resultAlloc = result.GetAllocator();
	result.AddMember("transactionList", Value(transactionListIt->value, resultAlloc), resultAlloc);
	return result;
}
