#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H

#include "gradido_blockchain/crypto/KeyPairEd25519.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"

//#include <string>

class Session : public MultithreadContainer
{
public:
	Session();
	~Session();

	//! \return 1 if user was existing
	//!	\return 0 if user was created		
	int loginOrCreate(const std::string& userName, const std::string& userPassword, const std::string& clientIp);
	inline const KeyPairEd25519* getKeyPair() const { std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex); return mUserKeyPair.get(); }
	bool verifyPassword(const std::string& password);
	inline const std::string& getClientIp() const { return mClientIp; }

	inline const unsigned char* getPublicKey() const { return !mUserKeyPair ? nullptr : mUserKeyPair->getPublicKey(); }
	bool signTransaction(model::gradido::GradidoTransaction* gradidoTransaction);

protected:

	void createNewUser(const std::string& userName);
	std::string mUserName;	
	std::string mClientIp;
	std::shared_ptr<SecretKeyCryptography> mEncryptionSecret;
	std::unique_ptr<KeyPairEd25519> mUserKeyPair;
	
};

class InvalidPasswordException : GradidoBlockchainException
{
public:
	explicit InvalidPasswordException(const char* what, const char* username, size_t passwordSize) noexcept;

	std::string getFullString() const;

protected:
	std::string mUsername;
	size_t mPasswordSize;
	
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H