#ifndef GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_SEND_TRANSACTION_IOTA_H
#define GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_SEND_TRANSACTION_IOTA_H

#include "http/JsonRequestHandler.h"

#include "rapidjson/document.h"

/*!
* @author Dario Rekowski
* @date 2021-12-16
* @brief create protobuf byte array from transaction details*
*/

class JsonSendTransactionIota : public JsonRequestHandler
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);

protected:
	
};


#endif //GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_SEND_TRANSACTION_IOTA_H