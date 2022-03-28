
#include "JsonRequestHandlerFactory.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/DateTimeFormatter.h"

#include "JsonPackTransaction.h"
#include "JsonCreationTransaction.h"
#include "JsonRegisterAddressTransaction.h"
#include "JsonSendTransactionIota.h"
#include "JsonUnknown.h"

#include <sstream>


JsonRequestHandlerFactory::JsonRequestHandlerFactory()
	: mRemoveGETParameters("^/([a-zA-Z0-9_-]*)"), mLogging(Poco::Logger::get("requestLog"))
{
}

Poco::Net::HTTPRequestHandler* JsonRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
	std::string uri = request.getURI();
	std::string url_first_part;
	std::stringstream logStream;

	mRemoveGETParameters.extract(uri, url_first_part);

	std::string dateTimeString = Poco::DateTimeFormatter::format(Poco::DateTime(), "%d.%m.%y %H:%M:%S");
	logStream << dateTimeString << " call " << uri;

	mLogging.information(logStream.str());

	auto client_host = request.clientAddress().host();
	//auto client_ip = request.clientAddress();
	// X-Real-IP forwarded ip from nginx config
	auto client_host_string = request.get("X-Real-IP", client_host.toString());
	client_host = Poco::Net::IPAddress(client_host_string);

	// check if user has valid session
	Poco::Net::NameValueCollection cookies;
	request.getCookies(cookies);

	int session_id = 0;

	try {
		session_id = atoi(cookies.get("GRADIDO_LOGIN").data());
	}
	catch (...) {}

	
	if (url_first_part == "/packTransaction") {
		return new JsonPackTransaction;
	}
	else if (url_first_part == "/sendTransactionIota") {
		return new JsonSendTransactionIota;
	}
	else if (url_first_part == "/creation") {
		return new JsonCreationTransaction;
	}
	else if (url_first_part == "/registerAddress") {
		return new JsonRegisterAddressTransaction;
	}
	

	return new JsonUnknown;
}

