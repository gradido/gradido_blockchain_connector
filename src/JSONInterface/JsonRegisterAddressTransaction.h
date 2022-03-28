#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_REGISTER_ADDRESS_TRANSACTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_REGISTER_ADDRESS_TRANSACTION_H

#include "JsonTransaction.h"

class JsonRegisterAddressTransaction : public JsonTransaction
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);

protected:
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_REGISTER_ADDRESS_TRANSACTION_H