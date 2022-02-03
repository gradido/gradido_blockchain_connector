#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_LIB_MOCK_IOTA_REQUEST_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_LIB_MOCK_IOTA_REQUEST_H

#include "http/IotaRequest.h"
#include "MockIotaHttpClientSession.h"

class MockIotaRequest : public IotaRequest
{
public:
	MockIotaRequest();

	Poco::SharedPtr<Poco::Net::HTTPClientSession> createClientSession();
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_LIB_MOCK_IOTA_REQUEST_H
