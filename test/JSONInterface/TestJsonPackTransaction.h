#ifndef __GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H
#define __GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H

#include "gtest/gtest.h"
#include "rapidjson/document.h"

#include "Poco/DateTime.h"

class TestJsonPackTransaction : public ::testing::Test
{

protected:
	void SetUp() override;
	void TearDown() override;


	// predefined transactions 
	/*
	{
		"transactionType": "transfer",
		"created": "2021-12-20 14:14:02",
		"memo": "Danke fuer deine Hilfe!",
		"senderPubkey": "131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6",
		"recipientPubkey": "eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"amount": 1000000
	}
	*/
	rapidjson::Document simpleTransfer(Poco::DateTime created);
	/*
	{
		"transactionType": "transfer",
		"created": "2021-12-20 14:14:02",
		"memo": "Danke fuer deine Hilfe!",
		"senderPubkey": "131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6",
		"recipientPubkey": "eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"amount": 1000000,
		"senderGroupAlias": "gdd1",
		"recipientGroupAlias": "gdd2"
	}
	*/
	rapidjson::Document crossGroupTransfer(Poco::DateTime created);
	/*
	{
		"transactionType": "creation",
		"created": "2021-11-05 10:30:10",
		"targetDate": "2021-10-01 00:00:00",
		"memo": "AGE Oktober 2021",
		"recipientPubkey": "eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"amount": 10000000
	}
	*/
	rapidjson::Document simpleCreation(Poco::DateTime now, Poco::DateTime targetDate);
	
};

#endif //__GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H