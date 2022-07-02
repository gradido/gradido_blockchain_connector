#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H

#include <unordered_map>
#include <memory>
#include "gradido_blockchain/crypto/KeyPairEd25519.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"
#include "../../task/Task.h"
#include "Poco/RefCountedObject.h"
#include "Poco/Tuple.h"

namespace model {
	namespace import {
		/*!
		  @author einhornimmond
		  @date   18.05.2022
		  @brief  Import user datas from old Login Server DB Backup
		*/
		class LoginServer: public Poco::RefCountedObject
		{
		public:
			LoginServer();
			virtual ~LoginServer();

			virtual void loadAll(const std::string& groupAlias);
			inline const std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>>& getUserKeys() const { return mUserKeys; }

			bool isAllRecoverKeyPairTasksFinished();
			virtual void cleanTransactions();
			//! \param pubkeyHex move string
			//! \return true if add to map worked
			bool addUserKey(std::unique_ptr<KeyPairEd25519> keyPair);
			const KeyPairEd25519* findUserKey(const std::string& pubkeyHex);

			KeyPairEd25519* findReserveKeyPair(const unsigned char* pubkey);
			const KeyPairEd25519* getOrCreateKeyPair(const std::string& originalPubkeyHex, const std::string& groupAlias);
			//! id, email, first name, last name, creation date
			typedef Poco::Tuple<Poco::UInt64, std::string, std::string, std::string, Poco::DateTime> UserTuple;
			virtual UserTuple getUserInfos(const std::string& pubkeyHex);

		protected:			
			const KeyPairEd25519* getReserveKeyPair(const std::string& originalPubkeyHex, const std::string& groupAlias);
			std::unique_ptr<KeyPairEd25519> createReserveKeyPair(const std::string& groupAlias, Poco::DateTime created = Poco::DateTime(2019, 10, 8, 10));

			void putReserveKeysIntoDb();

			//! key is original user pubkey hex
			std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>> mReserveKeyPairs;

			//! map key is user public key hex
			std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>> mUserKeys;
			Poco::AtomicCounter mLoadState;
			std::list<task::TaskPtr> mRecoverKeyPairTasks;
			mutable std::shared_mutex mWorkMutex;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H