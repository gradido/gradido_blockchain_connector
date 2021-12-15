#ifndef __GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H
#define __GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H

#include "gtest/gtest.h"

#include "Poco/JSON/Object.h"

class TestJsonPackTransaction : public ::testing::Test
{

protected:
	void SetUp() override;
	void TearDown() override;

	const proto::gradido::CrossGroupTransfer getCrossGroupTransfer(const proto::gradido::GradidoTransfer& protoTransfer) const;
};

#endif //__GRADIDO_LOGIN_SERVER_TEST_JSON_INTERFACE_TEST_JSON_PACK_TRANSACTION_H