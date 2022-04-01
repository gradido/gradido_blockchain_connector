#include "PasswordEncryptionRateLimiter.h"
#include "Poco/Environment.h"
#include "Poco/Thread.h"
#include "gradido_blockchain/lib/Profiler.h"

PasswordEncryptionRateLimiter::PasswordEncryptionRateLimiter()
{

}

PasswordEncryptionRateLimiter::~PasswordEncryptionRateLimiter()
{

}

PasswordEncryptionRateLimiter* PasswordEncryptionRateLimiter::getInstance()
{
	static PasswordEncryptionRateLimiter one;
	return &one;
}

void PasswordEncryptionRateLimiter::waitForFreeSlotAndRunEncryption(SecretKeyCryptography* secretCryptography, const std::string& username, const std::string& password)
{
	Profiler timeout;
	while (!checkForFreeSlotAndRunEncryption(secretCryptography, username, password)) {
		Poco::Thread::sleep(200);
		if (timeout.seconds() > 100) {
			throw PasswordEncryptionRateLimitException("cannot get free cpu core for encryption");
		}
	}
}

bool PasswordEncryptionRateLimiter::checkForFreeSlotAndRunEncryption(SecretKeyCryptography* secretCryptography, const std::string& username, const std::string& password)
{
	auto cpuCount = Poco::Environment::processorCount();
	lock();
	if (mRunningEncryptions.size() < cpuCount) {
		if (mRunningEncryptions.find(username) == mRunningEncryptions.end()) {
			mRunningEncryptions.insert(username);
			unlock();
			secretCryptography->createKey(username, password);
			lock();
			mRunningEncryptions.erase(username);
			unlock();
			return true;
		}
	}
	unlock();
	return false;
}