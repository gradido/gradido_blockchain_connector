#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_GLOBAL_GROUP_ADD_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_GLOBAL_GROUP_ADD_H

#include "JsonTransaction.h"

class JsonGlobalGroupAdd : public JsonTransaction
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_GLOBAL_GROUP_ADD_H