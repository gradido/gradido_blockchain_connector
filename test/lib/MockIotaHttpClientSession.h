#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_LIB_MOCK_IOTA_HTTP_CLIENT_SESSION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_LIB_MOCK_IOTA_HTTP_CLIENT_SESSION_H

// Poco::Net::HTTPClientSession
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"

#include <sstream>

class MockIotaHttpClientSession : public Poco::Net::HTTPClientSession
{
public:
	MockIotaHttpClientSession();

	virtual std::ostream& sendRequest(Poco::Net::HTTPRequest& request);
	virtual std::istream& receiveResponse(Poco::Net::HTTPResponse& response);

protected:
	std::stringstream mResponseStringbuf;
	std::istream mResponseStream;
	std::stringstream mRequestStringbuf;
	std::ostream mBuffer;
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_LIB_MOCK_IOTA_HTTP_CLIENT_SESSION_H