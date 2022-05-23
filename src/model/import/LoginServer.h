#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H

#include <unordered_map>
#include <memory>
#include "gradido_blockchain/crypto/KeyPairEd25519.h"

namespace model {
	namespace import {
		/*!
		  @author einhornimmond
		  @date   18.05.2022
		  @brief  Import user datas from old Login Server DB Backup
		*/
		class LoginServer
		{
		public:
			LoginServer();
			~LoginServer();

			void loadAll();

		protected:
			//! map key is user public key
			std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>> mUserKeys;
			Poco::AtomicCounter mLoadState;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_LOGIN_SERVER_H