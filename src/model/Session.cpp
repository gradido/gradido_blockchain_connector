#include "Session.h"

#include "table/User.h"
#include "table/UserBackup.h"

#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"
#include "gradido_blockchain/crypto/SealedBoxes.h"

Session::Session()
	: mUserKeyPair(nullptr)
{

}

Session::~Session()
{
	if (mUserKeyPair) {
		delete mUserKeyPair;
		mUserKeyPair = nullptr;
	}
}

int Session::loginOrCreate(const std::string& userName, const std::string& userPassword)
{
	std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
	auto users = model::table::getUserConnection();
	auto user = users.find_one(s::name = userName);

	mEncryptionSecret = std::make_shared<SecretKeyCryptography>();
	mEncryptionSecret->createKey(userName, userPassword);

	// if user don't exist, create him
	if (!user) {
		createNewUser(userName);
		return 0;
	}
	else {
		if (user->password != mEncryptionSecret->getKeyHashed()) {
			throw InvalidPasswordException("login failed", userName.data(), userPassword.size());
		}
		MemoryBin* privateKey = nullptr;
		if (SecretKeyCryptography::AUTH_DECRYPT_OK != mEncryptionSecret->decrypt(
			(const unsigned char*)user->encrypted_private_key.data(),
			user->encrypted_private_key.size(),
			&privateKey)) {
			// TODO: define exception classes for this
			throw Poco::NullPointerException("cannot decrypt private key");
		}
		mUserKeyPair = new KeyPairEd25519(privateKey);
		return 1;
	}
}



void Session::createNewUser(const std::string& userName)
{

	auto users = model::table::getUserConnection();
	auto userBackups = model::table::getUserBackupConnection();

	auto mm = MemoryManager::getInstance();
	auto passphrase = Passphrase::generate(&CryptoConfig::g_Mnemonic_WordLists[CryptoConfig::MNEMONIC_BIP0039_SORTED_ORDER]);
	mUserKeyPair = KeyPairEd25519::create(passphrase);
	
	auto cryptedPrivKey = mUserKeyPair->getCryptedPrivKey(mEncryptionSecret);
	if (!cryptedPrivKey) {
		// TODO: define exception classes for this
		throw Poco::NullPointerException("cannot get encrypted private key");
	}
	auto cryptedPrivKeyString = cryptedPrivKey->copyAsString();
	mm->releaseMemory(cryptedPrivKey);

	auto user_id = users.insert(
		s::name = userName,
		s::password = mEncryptionSecret->getKeyHashed(),
		s::public_key = std::string((const char*)mUserKeyPair->getPublicKey(), KeyPairEd25519::getPublicKeySize()),
		s::encrypted_private_key = *cryptedPrivKeyString.get()
	);
	

	if (!CryptoConfig::g_SupportPublicKey) {
		throw CryptoConfig::MissingKeyException("missing key for saving passphrase for new user", "supportPublicKey");
	}
	KeyPairEd25519 supportKeyPair(*CryptoConfig::g_SupportPublicKey);
	auto encryptedPassphrase = SealedBoxes::encrypt(&supportKeyPair, passphrase->getString());
	auto encryptedPassphraseString = encryptedPassphrase->copyAsString();
	mm->releaseMemory(encryptedPassphrase);

	userBackups.insert(
		s::user_id = user_id,
		s::key_user_id = 0,
		s::passphrase = *encryptedPassphraseString.get()
	);
	
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