#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_H

#include "gradido_blockchain/http/JsonRequestHandlerJwt.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"

#include "Poco/DateTime.h"
#include "../model/Session.h"

class JsonTransaction : public JsonRequestHandlerJwt
{
public:
protected:
	rapidjson::Document readSharedParameter(const rapidjson::Document& params);
	uint32_t readCoinColor(const rapidjson::Document& params);
	std::string signAndSendTransaction(std::unique_ptr<model::gradido::GradidoTransaction> transaction, const std::string& groupAlias);	

	rapidjson::Document handleSignAndSendTransactionExceptions();
	
	std::string mMemo;
	Poco::DateTime mCreated;
	uint64_t	mApolloTransactionId;
	std::string mApolloCreatedDecay;
	Poco::DateTime  mApolloDecayStart;
	Poco::SharedPtr<model::Session> mSession;
};



#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_H