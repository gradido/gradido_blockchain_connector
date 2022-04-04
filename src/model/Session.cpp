#include "Session.h"

#include "table/User.h"
#include "table/UserBackup.h"

#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"
#include "gradido_blockchain/crypto/SealedBoxes.h"

#include "PasswordEncryptionRateLimiter.h"
#include "ConnectionManager.h"

using namespace Poco::Data::Keywords;

Session::Session()
	: mUserKeyPair(nullptr)
{

}

Session::~Session()
{
}

int Session::loginOrCreate(const std::string& userName, const std::string& userPassword, const std::string& clientIp)
{
	mClientIp = clientIp;
	std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
	
	try {
		mEncryptionSecret = std::make_shared<SecretKeyCryptography>();
		PasswordEncryptionRateLimiter::getInstance()->waitForFreeSlotAndRunEncryption(mEncryptionSecret.get(), userName, userPassword);
		
		auto dbSession = ConnectionManager::getInstance()->getConnection();
		Poco::Data::Statement select(dbSession);
		uint64_t password(0);
		std::string encryptedPrivateKey;
		select << "SELECT password, encrypted_private_key from user where name = ?",
			into(password), into(encryptedPrivateKey), useRef(userName);
		if (select.execute()) {
			if (password != mEncryptionSecret->getKeyHashed()) {
				throw InvalidPasswordException("login failed", userName.data(), userPassword.size());
			}
			MemoryBin* privateKey = nullptr;
			if (SecretKeyCryptography::AUTH_DECRYPT_OK != mEncryptionSecret->decrypt(
				(const unsigned char*)encryptedPrivateKey.data(),
				encryptedPrivateKey.size(),
				&privateKey)) {
				// TODO: define exception classes for this
				throw Poco::NullPointerException("cannot decrypt private key");
			}
			mUserKeyPair = std::make_unique<KeyPairEd25519>(privateKey);
			return 1;
		}
		else {
			createNewUser(userName);
			return 0;
		}
		return -1;
	}
	catch (std::runtime_error& ex) {
		printf("runtime error: %s\n", ex.what());
		throw;
	}
}

bool Session::verifyPassword(const std::string& password)
{
	SecretKeyCryptography secretCryptography;
	PasswordEncryptionRateLimiter::getInstance()->waitForFreeSlotAndRunEncryption(&secretCryptography, mUserName, password);
	return mEncryptionSecret->getKeyHashed() == secretCryptography.getKeyHashed();
}



void Session::createNewUser(const std::string& userName)
{	
	auto mm = MemoryManager::getInstance();
	auto passphrase = Passphrase::generate(&CryptoConfig::g_Mnemonic_WordLists[CryptoConfig::MNEMONIC_BIP0039_SORTED_ORDER]);
	mUserKeyPair = std::move(KeyPairEd25519::create(passphrase));
	
	auto cryptedPrivKey = mUserKeyPair->getCryptedPrivKey(mEncryptionSecret);
	if (!cryptedPrivKey) {
		// TODO: define exception classes for this
		throw Poco::NullPointerException("cannot get encrypted private key");
	}
	printf("key hashed: %d\n", mEncryptionSecret->getKeyHashed());

	
	model::table::User user(
		userName, 
		mEncryptionSecret->getKeyHashed(), 
		mUserKeyPair->getPublicKey(),
		cryptedPrivKey
	);
	mm->releaseMemory(cryptedPrivKey);
	auto userId = user.save();


	if (!CryptoConfig::g_SupportPublicKey) {
		throw CryptoConfig::MissingKeyException("missing key for saving passphrase for new user", "supportPublicKey");
	}
	AuthenticatedEncryption supportKeyPair(*CryptoConfig::g_SupportPublicKey);
	auto encryptedPassphrase = SealedBoxes::encrypt(&supportKeyPair, passphrase->getString());

	model::table::UserBackup userBackup(
		userId, 
		0, 
		encryptedPassphrase
	);

	mm->releaseMemory(encryptedPassphrase);

	userBackup.save();	
}

bool Session::signTransaction(model::gradido::GradidoTransaction* gradidoTransaction)
{
	auto mm = MemoryManager::getInstance();
	try {
		auto sign = mUserKeyPair->sign(*gradidoTransaction->getTransactionBody()->getBodyBytes().get());
		auto pubkey = mUserKeyPair->getPublicKeyCopy();
		gradidoTransaction->addSign(pubkey, sign);
		mm->releaseMemory(sign);
		mm->releaseMemory(pubkey);
	}
	catch (Ed25519SignException& ex) {
		Poco::Logger::get("errorLog").error("[Session::signTransaction] error signing transaction: %s", ex.getFullString());
		return false;
	}

	return true;
}


// +++++++++++++++++++ Invalid Password Exception ++++++++++++++++++++++++++++
InvalidPasswordException::InvalidPasswordException(const char* what, const char* username, size_t passwordSize) noexcept
	: GradidoBlockchainException(what), mUsername(username), mPasswordSize(passwordSize)
{

}

std::string InvalidPasswordException::getFullString() const
{
	std::string result = what();
	result += ", username: " + mUsername;
	result += ", password size: " + mPasswordSize;
	return result;
}