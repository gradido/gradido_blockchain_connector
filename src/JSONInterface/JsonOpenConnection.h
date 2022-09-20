#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_OPEN_CONNECTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_OPEN_CONNECTION_H

#include "gradido_blockchain/http/JsonRequestHandler.h"

class JsonOpenConnection : public JsonRequestHandler
{
public:
	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);

};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_OPEN_CONNECTION_H