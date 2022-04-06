#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_LISTENER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_LISTENER_H

#include "gradido_blockchain/http/JsonRequestHandler.h"

// called from Gradido Node after it has processed a new transaction
class JsonTransactionListener : public JsonRequestHandler
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);

};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_LISTENER_H