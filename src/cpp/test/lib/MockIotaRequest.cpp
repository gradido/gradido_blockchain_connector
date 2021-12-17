#include "MockIotaRequest.h"


MockIotaRequest::MockIotaRequest()
	: IotaRequest("", 80, "")
{

}

Poco::SharedPtr<Poco::Net::HTTPClientSession> MockIotaRequest::createClientSession(NotificationList* errorReciver)
{
	Poco::SharedPtr<Poco::Net::HTTPClientSession> mockedClientSession(new MockIotaHttpClientSession);
	return mockedClientSession;	
}