
#include "gtest/gtest.h"

#include "JSONInterface/JsonPackTransaction.h"
#include "lib/DataTypeConverter.h"
#include "TestJsonPackTransaction.h"

#include "Poco/DateTimeFormatter.h"
#include "proto/gradido/TransactionBody.pb.h"

#include "rapidjson/document.h"

using namespace rapidjson;


void TestJsonPackTransaction::SetUp()
{
	
}

void TestJsonPackTransaction::TearDown()
{
	
}

/*
{
	"state": "success",
	"transactions": [
		{
			"bodyBytesBase64": "ChdEYW5rZSBmw7xyIGRlaW5lIEhpbGZlIRIGCKCg6/8FMkwKSgomCiATHH9o3ZSyvkyRNAD/f/TNwDrCvamcLSntyss7Blxn5hCAiXoSIO/3pKRA6xD6bVrl7kfWMkDFXqPhly6YFcCUEeJasJ/d"
		}
	]
}
*/

TEST_F(TestJsonPackTransaction, LocalTransfer)
{
	Document params(kObjectType);
	auto alloc = params.GetAllocator();
	params.AddMember("transactionType", "transfer", alloc);
	Poco::DateTime now;
	std::string memo = "Danke fuer deine Hilfe!";
	params.AddMember("created", Value(Poco::DateTimeFormatter::format(now, "%Y-%m-%d %H:%M:%S").data(), alloc), alloc);
	params.AddMember("memo", Value(memo.data(), alloc), alloc);
	params.AddMember("senderPubkey", "131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6", alloc);
	params.AddMember("recipientPubkey", "eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd", alloc);
	params.AddMember("amount", 1000000, alloc);
	
	JsonPackTransaction jsonCall;
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

	auto transactionsIt = result.FindMember("transactions");
	ASSERT_TRUE(transactionsIt != result.MemberEnd());

	ASSERT_TRUE(transactionsIt->value.IsArray());
	auto transaction = transactionsIt->value.Begin();
	ASSERT_FALSE(transaction->IsNull());
	ASSERT_TRUE(transaction->HasMember("bodyBytesBase64"));
	auto bodyBytesIt = transaction->FindMember("bodyBytesBase64");
	ASSERT_TRUE(bodyBytesIt->value.IsString());

	std::string base64BodyBytes = bodyBytesIt->value.GetString();

	auto bodyBytes = DataTypeConverter::base64ToBin(base64BodyBytes);

	proto::gradido::TransactionBody protoBody;
	ASSERT_TRUE(protoBody.ParseFromString(std::string((const char*)bodyBytes->data(), bodyBytes->size())));
	ASSERT_TRUE(protoBody.has_transfer());
	auto nowRead = DataTypeConverter::convertFromProtoTimestampSeconds(protoBody.created());
	Poco::DateTime readDate(nowRead);
	if (now.timestamp().epochTime() != nowRead.epochTime()) {
		std::clog << "input: " << Poco::DateTimeFormatter::format(now, "%Y-%m-%d %H:%M:%S") << std::endl;
		std::clog << "out  : " << Poco::DateTimeFormatter::format(readDate, "%Y-%m-%d %H:%M:%S") << std::endl;
	}
	ASSERT_EQ(now.timestamp().epochTime(), nowRead.epochTime());
	ASSERT_EQ(memo, protoBody.memo());

	MemoryManager::getInstance()->releaseMemory(bodyBytes);
}