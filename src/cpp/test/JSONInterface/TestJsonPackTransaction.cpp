﻿
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

const proto::gradido::CrossGroupTransfer TestJsonPackTransaction::getCrossGroupTransfer(const proto::gradido::GradidoTransfer& protoTransfer) const
{
	if (protoTransfer.has_inbound()) {
		return protoTransfer.inbound();
	}
	else if (protoTransfer.has_outbound()) {
		return protoTransfer.outbound();
	}
	throw std::runtime_error("invalid cross group transfer transaction");
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
	auto transfer = protoBody.transfer();
	ASSERT_EQ(transfer.local().sender().amount(), 1000000);
	ASSERT_EQ(
		DataTypeConverter::binToHex(transfer.local().sender().pubkey()).substr(0, 64),
		"131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6"
	);
	ASSERT_EQ(
		DataTypeConverter::binToHex(transfer.local().recipiant()).substr(0, 64),
		"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd"
	);
	auto nowRead = DataTypeConverter::convertFromProtoTimestampSeconds(protoBody.created());
	Poco::DateTime readDate(nowRead);
	ASSERT_EQ(now.timestamp().epochTime(), nowRead.epochTime());
	ASSERT_EQ(memo, protoBody.memo());

	MemoryManager::getInstance()->releaseMemory(bodyBytes);
}


TEST_F(TestJsonPackTransaction, CrossGroupTransfer)
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
	params.AddMember("senderGroupAlias", "gdd1", alloc);
	params.AddMember("recipientGroupAlias", "gdd2", alloc);

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
	int i = 0;
	for (auto transactionIt = transactionsIt->value.Begin(); transactionIt != transactionsIt->value.End(); transactionIt++) {
		ASSERT_FALSE(transactionIt->IsNull());
		ASSERT_TRUE(transactionIt->HasMember("bodyBytesBase64"));
		ASSERT_TRUE(transactionIt->HasMember("groupAlias"));
		auto bodyBytesIt = transactionIt->FindMember("bodyBytesBase64");
		auto groupAliasIt = transactionIt->FindMember("groupAlias");
		ASSERT_TRUE(bodyBytesIt->value.IsString());
		ASSERT_TRUE(groupAliasIt->value.IsString());

		auto base64BodyBytes = bodyBytesIt->value.GetString();
		auto groupAlias = groupAliasIt->value.GetString();

		auto bodyBytes = DataTypeConverter::base64ToBin(base64BodyBytes);

		proto::gradido::TransactionBody protoBody;
		ASSERT_TRUE(protoBody.ParseFromString(std::string((const char*)bodyBytes->data(), bodyBytes->size())));
		MemoryManager::getInstance()->releaseMemory(bodyBytes);

		ASSERT_TRUE(protoBody.has_transfer());
		auto transfer = protoBody.transfer();
		auto crossGroup = getCrossGroupTransfer(protoBody.transfer());

		ASSERT_EQ(crossGroup.transfer().sender().amount() , 1000000);
		ASSERT_EQ(
			DataTypeConverter::binToHex(crossGroup.transfer().sender().pubkey()).substr(0,64),
			"131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6"
		);
		ASSERT_EQ(
			DataTypeConverter::binToHex(crossGroup.transfer().recipiant()).substr(0,64),
			"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd"
		);

		if (i == 0) {
			ASSERT_EQ(groupAlias, std::string("gdd1"));
			ASSERT_EQ(crossGroup.other_group(), std::string("gdd2"));
		}
		else if (i == 1) {
			ASSERT_EQ(groupAlias, std::string("gdd2"));
			ASSERT_EQ(crossGroup.other_group(), std::string("gdd1"));
		}

		auto nowRead = DataTypeConverter::convertFromProtoTimestampSeconds(protoBody.created());
		Poco::DateTime readDate(nowRead);
		ASSERT_EQ(now.timestamp().epochTime(), nowRead.epochTime());
		ASSERT_EQ(memo, protoBody.memo());
		
		i++;
	}
	
}

TEST_F(TestJsonPackTransaction, Creation)
{
	Document params(kObjectType);
	auto alloc = params.GetAllocator();
	params.AddMember("transactionType", "creation", alloc);
	// int year, int month, int day
	std::string memo = "AGE Oktober 2021";
	Poco::DateTime created(2021, 11, 5, 10, 30, 10);
	Poco::DateTime targetDate(2021, 10, 1);
	params.AddMember("created", Value(Poco::DateTimeFormatter::format(created, "%Y-%m-%d %H:%M:%S").data(), alloc), alloc);
	params.AddMember("targetDate", Value(Poco::DateTimeFormatter::format(targetDate, "%Y-%m-%d %H:%M:%S").data(), alloc), alloc);
	params.AddMember("memo", Value(memo.data(), alloc), alloc);
	params.AddMember("recipientPubkey", "eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd", alloc);
	params.AddMember("amount", 10000000, alloc);

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
	ASSERT_TRUE(protoBody.has_creation());
	auto creation = protoBody.creation();
	ASSERT_EQ(creation.recipiant().amount(), 10000000);
	ASSERT_EQ(
		DataTypeConverter::binToHex(creation.recipiant().pubkey()).substr(0, 64),
		"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd"
	);
	auto nowRead = DataTypeConverter::convertFromProtoTimestampSeconds(protoBody.created());
	auto targetDateRead = DataTypeConverter::convertFromProtoTimestampSeconds(creation.target_date());
	Poco::DateTime readDate(nowRead);
	ASSERT_EQ(created.timestamp().epochTime(), nowRead.epochTime());
	ASSERT_EQ(targetDate.timestamp().epochTime(), targetDateRead.epochTime());
	ASSERT_EQ(memo, protoBody.memo());

	MemoryManager::getInstance()->releaseMemory(bodyBytes);
}