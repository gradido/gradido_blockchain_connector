#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_GET_PENDING_TRANSACTIONS_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_GET_PENDING_TRANSACTIONS_H

#include "gradido_blockchain/http/JsonRequestHandler.h"

class JsonGetPendingTransactions : public JsonRequestHandler
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);

protected:
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_GET_PENDING_TRANSACTIONS_H