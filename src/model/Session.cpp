#include "Session.h"

#include "table/User.h"
#include "table/Group.h"
#include "table/UserBackup.h"

#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"
#include "gradido_blockchain/crypto/SealedBoxes.h"
#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "gradido_blockchain/http/RequestExceptions.h"

#include "PasswordEncryptionRateLimiter.h"
#include "ConnectionManager.h"

using namespace Poco::Data::Keywords;
using namespace rapidjson;

namespace model {

	Session::Session()
		: mUserKeyPair(nullptr), mGroupId(0)
	{

	}

	Session::~Session()
	{
	}

	int Session::loginOrCreate(const std::string& userName, const std::string& userPassword, const std::string& groupAlias, const std::string& clientIp)
	{
		mClientIp = clientIp;
		mGroupAlias = groupAlias;
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);

		try {
			mEncryptionSecret = std::make_shared<SecretKeyCryptography>();
			PasswordEncryptionRateLimiter::getInstance()->waitForFreeSlotAndRunEncryption(mEncryptionSecret.get(), userName, userPassword);

			auto dbSession = ConnectionManager::getInstance()->getConnection();
			// load group id or create new entry if not already exist
			getGroupId(dbSession);
			Poco::Data::Statement select(dbSession);
			uint64_t password(0);
			std::string encryptedPrivateKey;
			select << "SELECT password, encrypted_private_key from user where name = ? AND group_id = ?",
				into(password), into(encryptedPrivateKey), useRef(userName), useRef(mGroupId);
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
				createNewUser(userName, groupAlias, dbSession);
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

	uint64_t Session::getGroupId(Poco::Data::Session& dbSession)
	{
		if (mGroupId) return mGroupId;
		try {
			auto group = table::Group::load(mGroupAlias);
			mGroupId = group->getId();
			return mGroupId;
		}
		catch (table::RowNotFoundException& ex) {
			// create new group if group with alias not exist
			// load details from blockchain
			JsonRPCRequest askGroupDetails(ServerConfig::g_GradidoNodeUri);
			Value params(kObjectType);
			params.AddMember("groupAlias", Value(mGroupAlias.data(), askGroupDetails.getJsonAllocator()), askGroupDetails.getJsonAllocator());
			std::string groupName = mGroupAlias;
			uint32_t coinColor = 0;
			try {
				auto result = askGroupDetails.request("getgroupdetails", params);
				groupName = result["groupName"].GetString();
				coinColor = result["coinColor"].GetUint();				
			}
			catch (JsonRPCException& ex) {
				Poco::Logger::get("errroLog").information("group: %s not exist on blockchain, request result: %s", mGroupAlias, ex.getFullString());
			}
			auto group = std::make_unique<table::Group>(groupName, mGroupAlias, "", coinColor);
			group->save(dbSession);
			mGroupId = group->getLastInsertId(dbSession);
			return mGroupId;
		}
	}

	void Session::createNewUser(const std::string& userName, const std::string& groupAlias, Poco::Data::Session& dbSession)
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

		table::User user(
			userName,
			mGroupId,
			mEncryptionSecret->getKeyHashed(),
			mUserKeyPair->getPublicKey(),
			cryptedPrivKey
		);
		mm->releaseMemory(cryptedPrivKey);
		user.save(dbSession);
		auto userId = user.getLastInsertId(dbSession);

		if (!CryptoConfig::g_SupportPublicKey) {
			throw CryptoConfig::MissingKeyException("missing key for saving passphrase for new user", "supportPublicKey");
		}
		AuthenticatedEncryption supportKeyPair(*CryptoConfig::g_SupportPublicKey);
		auto encryptedPassphrase = SealedBoxes::encrypt(&supportKeyPair, passphrase->getString());

		table::UserBackup userBackup(
			userId,
			0,
			encryptedPassphrase
		);

		mm->releaseMemory(encryptedPassphrase);
		userBackup.save(dbSession);
	}

	bool Session::signTransaction(model::gradido::GradidoTransaction* gradidoTransaction)
	{
		auto mm = MemoryManager::getInstance();
		try {
			printf("body bytes for signing: \n%s\n", DataTypeConverter::binToBase64(*gradidoTransaction->getTransactionBody()->getBodyBytes().get()).data());
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

}