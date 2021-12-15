#ifndef GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_PACK_TRANSACTION_H
#define GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_PACK_TRANSACTION_H

#include "JsonRequestHandler.h"
#include "model/gradido/Transaction.h"

/*!
* @author Dario Rekowski
* @date 2021-12-13
* @brief create protobuf byte array from transaction details*
*/

class JsonPackTransaction : public JsonRequestHandler
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);

protected:
	typedef std::pair<Poco::AutoPtr<model::gradido::Transaction>, std::string> TransactionGroupAlias;

	rapidjson::Document transfer(const rapidjson::Document& params);
	rapidjson::Document creation(const rapidjson::Document& params);
	rapidjson::Document groupMemberUpdate(const rapidjson::Document& params);

	rapidjson::Document resultBase64Transactions(std::vector<TransactionGroupAlias> transactions);

	std::string mMemo;
	Poco::DateTime mCreated;
};


#endif //GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_PACK_TRANSACTION_H