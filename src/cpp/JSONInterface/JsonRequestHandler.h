#ifndef __JSON_INTERFACE_JSON_REQUEST_HANDLER_
#define __JSON_INTERFACE_JSON_REQUEST_HANDLER_

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/URI.h"
#include "../lib/NotificationList.h"

#include "rapidjson/document.h"


class JsonRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:

	JsonRequestHandler();
	JsonRequestHandler(Poco::Net::IPAddress clientIp);

	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);

	virtual rapidjson::Document handle(const rapidjson::Document& params) { return rapidjson::Document(); };

	void parseJsonWithErrorPrintFile(std::istream& request_stream, rapidjson::Document& rapidParams, NotificationList* errorHandler = nullptr, const char* functionName = nullptr);
	static bool parseQueryParametersToRapidjson(const Poco::URI& uri, rapidjson::Document& rapidParams);


	static rapidjson::Document getIntParameter(const rapidjson::Document& params, const char* fieldName, int& iParameter);
	static rapidjson::Document getUIntParameter(const rapidjson::Document& params, const char* fieldName, unsigned int& iParameter);
	static rapidjson::Document getBoolParameter(const rapidjson::Document& params, const char* fieldName, bool& bParameter);
	static rapidjson::Document getUInt64Parameter(const rapidjson::Document& params, const char* fieldName, Poco::UInt64& iParameter);
	static rapidjson::Document getStringParameter(const rapidjson::Document& params, const char* fieldName, std::string& strParameter);
	static rapidjson::Document getStringIntParameter(const rapidjson::Document& params, const char* fieldName, std::string& strParameter, int& iParameter);
	static rapidjson::Document checkArrayParameter(const rapidjson::Document& params, const char* fieldName);
	static rapidjson::Document checkObjectParameter(const rapidjson::Document& params, const char* fieldName);


	static rapidjson::Document stateError(const char* msg, std::string details = "");
	static rapidjson::Document stateError(const char* msg, NotificationList* errorReciver);
	static rapidjson::Document customStateError(const char* state, const char* msg, std::string details = "");
	static rapidjson::Document stateSuccess();
	static rapidjson::Document stateWarning(const char* msg, std::string details = "");

protected:
	Poco::Net::IPAddress mClientIp;
	std::string			mServerHost;
	
	bool getTargetGroup(const rapidjson::Document& params);

	
};

#endif // __JSON_INTERFACE_JSON_REQUEST_HANDLER_