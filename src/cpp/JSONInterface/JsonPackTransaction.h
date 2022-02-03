#ifndef GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_PACK_TRANSACTION_H
#define GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_PACK_TRANSACTION_H

#include "http/JsonRequestHandler.h"
#include "model/TransactionFactory.h"
#include "model/CrossGroupTransactionBuilder.h"

/*!
* @author Dario Rekowski
* @date 2021-12-13
* @brief create protobuf byte array from transaction details
*/

class JsonPackTransaction : public JsonRequestHandler
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);

protected:
	typedef std::pair<model::gradido::GradidoTransaction*, std::string> TransactionGroupAlias;

	rapidjson::Document transfer(const rapidjson::Document& params);
	rapidjson::Document creation(const rapidjson::Document& params);
	rapidjson::Document registerAddress(const rapidjson::Document& params);

	rapidjson::Document resultBase64Transactions(std::vector<TransactionGroupAlias> transactions);

	uint32_t readCoinColor(const rapidjson::Document& params);

	std::string mMemo;
	Poco::DateTime mCreated;
};


#endif //GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_PACK_TRANSACTION_H