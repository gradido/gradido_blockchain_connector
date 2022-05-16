#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_H

#include "gradido_blockchain/http/JsonRequestHandlerJwt.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"

#include "Poco/DateTime.h"
#include "../model/Session.h"

class JsonTransaction : public JsonRequestHandlerJwt
{
public:
	JsonTransaction();
protected:
	rapidjson::Document readSharedParameter(const rapidjson::Document& params);
	std::string signAndSendTransaction(std::unique_ptr<model::gradido::GradidoTransaction> transaction, const std::string& groupAlias);	
	bool validateApolloDecay(const model::gradido::GradidoTransaction* gradidoTransaction);
	rapidjson::Document handleSignAndSendTransactionExceptions();

	//! \return base64 encoded encrypted memo
	static std::string encryptMemo(const std::string& memo, const unsigned char* ed25519Pubkey, const MemoryBin* ed25519Privkey);
	static std::string decryptMemo(const std::string& base64EncodedEncryptedMemo, const unsigned char* ed25519Pubkey, const MemoryBin* ed25519Privkey);
	
	std::string mMemo;
	Poco::DateTime mCreated;
	uint64_t	mApolloTransactionId;
	std::string mApolloCreatedDecay;
	Poco::DateTime  mApolloDecayStart;
	bool		mArchiveTransaction;
	uint64_t    mTransactionNr;
	Poco::SharedPtr<model::Session> mSession;
};

class ApolloDecayException : public GradidoBlockchainException
{
public:
	explicit ApolloDecayException(const char* what, std::string startBalance, std::string decay) noexcept;
	std::string getFullString() const;
	rapidjson::Value getDetails(rapidjson::Document::AllocatorType& alloc) const;

protected:
	std::string mStartBalance;
	std::string mDecay;

};



#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_JSON_TRANSACTION_H