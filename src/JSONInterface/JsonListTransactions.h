#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_LIST_TRANSACTIONS_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_LIST_TRANSACTIONS_H

#include "JsonTransaction.h"

class JsonListTransactions: public JsonTransaction
{
public:
	rapidjson::Document handle(const rapidjson::Document& params);

protected:
	void loadUsersFromDB();
	std::unordered_map<std::string, std::string> mHexPubkeyUserNames;
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_LIST_TRANSACTIONS_H