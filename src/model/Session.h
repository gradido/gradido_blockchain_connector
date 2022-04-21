#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H

#include "gradido_blockchain/crypto/KeyPairEd25519.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"

#include "model/table/Group.h"
#include "Poco/Data/Session.h"

namespace model {

	class Session : public MultithreadContainer
	{
	public:
		Session();
		~Session();

		//! \return 1 if user was existing
		//!	\return 0 if user was created		
		int loginOrCreate(const std::string& userName, const std::string& userPassword, const std::string& groupAlias, const std::string& clientIp);
		inline const KeyPairEd25519* getKeyPair() const { std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex); return mUserKeyPair.get(); }
		bool verifyPassword(const std::string& password);
		inline const std::string& getClientIp() const { return mClientIp; }
		inline const std::string& getGroupAlias() const { return mGroupAlias; }
		inline uint64_t getGroupId() const { return mGroupId; }
		uint64_t getGroupId(Poco::Data::Session& dbSession);

		inline const unsigned char* getPublicKey() const { return !mUserKeyPair ? nullptr : mUserKeyPair->getPublicKey(); }
		bool signTransaction(model::gradido::GradidoTransaction* gradidoTransaction);

	protected:
		std::unique_ptr<model::table::Group> askForGroupDetails();
		void createNewUser(const std::string& userName, const std::string& groupAlias, Poco::Data::Session& dbSession);

		std::string mUserName;
		std::string mGroupAlias;
		int			mGroupId;
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

}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_SESSION_H