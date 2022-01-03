
#include "gtest/gtest.h"

#include "JSONInterface/JsonSendTransactionIota.h"
#include "lib/DataTypeConverter.h"
#include "model/gradido/Transaction.h"
#include "TestJsonSendTransactionIota.h"

#include "Poco/DateTimeFormatter.h"
#include "proto/gradido/TransactionBody.pb.h"
#include "ServerConfig.h"

#include "../lib/MockIotaRequest.h"

#include "rapidjson/document.h"

using namespace rapidjson;


void TestJsonSendTransactionIota::SetUp()
{
	mPreviousIotaRequest = nullptr;
	mPreviousIotaRequest = ServerConfig::g_IotaRequestHandler;
	ServerConfig::g_IotaRequestHandler = new MockIotaRequest;
}

void TestJsonSendTransactionIota::TearDown()
{
	delete ServerConfig::g_IotaRequestHandler;
	ServerConfig::g_IotaRequestHandler = mPreviousIotaRequest;
}

/*
	"bodyBytesBase64": "CAEStwEKZgpkCiDs2zemYO1PxD1Odwh5YxyUmDp+lyxgVmiQgiFdPLUahRJAvyYRVASJNvyYiTAT2D8t6QtVgekqnsPIJRAx6jG8tEqxdzwKGg/Jm0gatdY1Ix7DGbHMBRw/9CtoXQXueqqDChJNChBBR0UgT2t0b2JlciAyMDIxEgYIuMWpjQY6MQonCiAdkWfcgRcDfCIg+GbikK6U9Fp4WMTGtAxF7RRdvhisSxCA2sQJGgYIgJ/ZigYaBgjGxamNBiABKiCijBRel5hudg5iZqfeQxjzIMhnOJA+tmHmloMVW+snjTIEyWETAA==",
	"signaturePairs": [
		{
			"pubkey": "ecdb37a660ed4fc43d4e770879631c94983a7e972c6056689082215d3cb51a85",
			"signature": "c1b2e8077c206bf78aeaefbdcdfe8a5ae32ddf9adca95ba1f5a93e885b7b3a00f224fe1fb0ea491d20966f7336fd90479c792432dc94b8c8b83dd00510fca508"
		}
	],
	"groupAlias":"gdd1"
*/

TEST_F(TestJsonSendTransactionIota, SendValidTransaction)
{ 
	auto mm = MemoryManager::getInstance();
	auto public_key = mm->getFreeMemory(crypto_sign_PUBLICKEYBYTES);
	auto secrect_key = mm->getFreeMemory(crypto_sign_SECRETKEYBYTES);
	auto sign = mm->getFreeMemory(crypto_sign_BYTES);
	crypto_sign_keypair(*public_key, *secrect_key);
	
	//
	auto transaction = model::gradido::Transaction::createGroupMemberUpdateAdd(public_key);
	auto bodyBytes = transaction->getTransactionBody()->getBodyBytes();
	crypto_sign_detached(*sign, NULL, (const unsigned char*)bodyBytes.data(), bodyBytes.size(), *secrect_key);

	Document params(kObjectType);
	auto alloc = params.GetAllocator();
	Value signaturePairs(kArrayType);
	
	params.AddMember("bodyBytesBase64", Value(DataTypeConverter::binToBase64(bodyBytes).data(), alloc), alloc);
	
	params.AddMember("groupAlias", "gdd1", alloc);

	Value entry(kObjectType);
	entry.AddMember("pubkey", Value(public_key->convertToHex().data(), alloc), alloc);
	entry.AddMember("signature", Value(sign->convertToHex().data(), alloc), alloc);
	signaturePairs.PushBack(entry, alloc);

	params.AddMember("signaturePairs", signaturePairs, alloc);

	mm->releaseMemory(public_key);
	mm->releaseMemory(secrect_key);
	mm->releaseMemory(sign);

	JsonSendTransactionIota jsonCall;
	auto result = jsonCall.handle(params);

	std::string state;
	jsonCall.getStringParameter(result, "state", state);
	if (state != "success") {
		std::string msg;
		std::string details;
		jsonCall.getStringParameter(result, "msg", msg);
		jsonCall.getStringParameter(result, "details", details);
		std::clog << "msg: " << msg;
		if (details.size()) {
			std::clog << ", details: " << details;
		}
		std::clog << std::endl;
	}
	ASSERT_EQ(state, "success");
	std::string messageId;
	jsonCall.getStringParameter(result, "iotaMessageId", messageId);
	ASSERT_EQ(messageId.size(), 64);
}

