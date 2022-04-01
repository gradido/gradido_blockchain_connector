#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_PASSWORD_ENCRYPTION_RATE_LIMITER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_PASSWORD_ENCRYPTION_RATE_LIMITER_H

#include <unordered_set>
#include "gradido_blockchain/crypto/SecretKeyCryptography.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"

class PasswordEncryptionRateLimiter : public MultithreadContainer
{
public:
	~PasswordEncryptionRateLimiter();

	static PasswordEncryptionRateLimiter* getInstance();
	void waitForFreeSlotAndRunEncryption(SecretKeyCryptography* secretCryptography, const std::string& username, const std::string& password);

protected:
	bool checkForFreeSlotAndRunEncryption(SecretKeyCryptography* secretCryptography, const std::string& username, const std::string& password);

	PasswordEncryptionRateLimiter();
	std::unordered_set<std::string> mRunningEncryptions;

};

class PasswordEncryptionRateLimitException : public GradidoBlockchainException
{
public:
	explicit PasswordEncryptionRateLimitException(const char* what) noexcept
		: GradidoBlockchainException(what) {}
	std::string getFullString() const { return what(); }

protected:
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_PASSWORD_ENCRYPTION_RATE_LIMITER_H