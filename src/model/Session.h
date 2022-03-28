#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H

#include "gradido_blockchain/crypto/KeyPairEd25519.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"

//#include <string>

class Session : public MultithreadContainer
{
public:
	Session();
	~Session();

	//! \return 1 if user was existing
	//!	\return 0 if user was created		
	int loginOrCreate(const std::string& userName, const std::string& userPassword);
	inline const KeyPairEd25519* getKeyPair() const { std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex); return mUserKeyPair; }


protected:

	void createNewUser(const std::string& userName);
	std::string mUserName;
	std::shared_ptr<SecretKeyCryptography> mEncryptionSecret;
	KeyPairEd25519* mUserKeyPair;
	
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