#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H

#include <unordered_map>
#include <memory>
#include "gradido_blockchain/crypto/KeyPairEd25519.h"
#include "gradido_blockchain/lib/MultithreadContainer.h"
#include "../../task/Task.h"
#include "Poco/RefCountedObject.h"

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
			~LoginServer();

			void loadAll(const std::string& groupAlias);
			inline const std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>>& getUserKeys() const { return mUserKeys; }

			bool isAllRecoverKeyPairTasksFinished();
			void cleanTransactions();
			//! \param pubkeyHex move string
			//! \return true if add to map worked
			bool addUserKeys(std::unique_ptr<KeyPairEd25519> keyPair);

		protected:
			//! map key is user public key hex
			std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>> mUserKeys;
			Poco::AtomicCounter mLoadState;
			std::list<task::TaskPtr> mRecoverKeyPairTasks;
			std::shared_mutex mWorkMutex;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H