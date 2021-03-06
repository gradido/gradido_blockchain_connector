#ifndef __GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H
#define __GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H

#include "gtest/gtest.h"
#include "gradido_blockchain/http/IotaRequest.h"

class TestJsonSendTransactionIota : public ::testing::Test
{
protected:
	void SetUp() override;
	void TearDown() override;

	IotaRequest* mPreviousIotaRequest;
};

#endif //__GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H