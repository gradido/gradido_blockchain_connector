#include "Session.h"

#include "table/User.h"
#include "table/UserBackup.h"

#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"
#include "gradido_blockchain/crypto/SealedBoxes.h"

#include "PasswordEncryptionRateLimiter.h"

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
		
		auto dbConnection = ServerConfig::g_Mysql->connect();
		auto preparedStatement = dbConnection.prepare("SELECT password, encrypted_private_key from user where name = ?");
		auto optional_result = preparedStatement(userName).read_optional<uint64_t, li::sql_blob>();
		if (optional_result) {
			auto [password, encrypted_private_key] = optional_result.value();
			printf("password: %d, encrypted priv key: %s\n", password, DataTypeConverter::binToHex((const unsigned char*)encrypted_private_key.data(), encrypted_private_key.size()).data());

			if (password != mEncryptionSecret->getKeyHashed()) {
				throw InvalidPasswordException("login failed", userName.data(), userPassword.size());
			}
			MemoryBin* privateKey = nullptr;
			if (SecretKeyCryptography::AUTH_DECRYPT_OK != mEncryptionSecret->decrypt(
				(const unsigned char*)encrypted_private_key.data(),
				encrypted_private_key.size(),
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