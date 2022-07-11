#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_RECOVER_LOGIN_KEY_PAIR_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_RECOVER_LOGIN_KEY_PAIR_H

#include "CPUTask.h"
#include "../model/import/LoginServer.h"
#include "Poco/AutoPtr.h"

namespace task
{
	/*!
	@author einhornimmond
	
	@brief create key pair from passphrase 

		- create key pair from passphrase 
		- if original pubkey was set: 
			- compare calculated public key with stored public key for the user
	*/ 
	class RecoverLoginKeyPair : public CPUTask
	{
	public:
		//! \param passphrase move string here
		RecoverLoginKeyPair(
			uint64_t id, 
			std::string passphrase, 
			Poco::AutoPtr<model::import::LoginServer> loginServer,
			Poco::DateTime created,
			const std::string& groupAlias
		);
		~RecoverLoginKeyPair();

		const char* getResourceType() const { return "RecoverLoginKeyPair"; };
		int run();
		inline void setOriginalPubkeyHex(const std::string& pubkeyHex) { mOriginalPubkey = pubkeyHex; }
	protected:
		uint64_t mId;
		std::string mPassphrase;
		std::string mOriginalPubkey;
		Poco::AutoPtr<model::import::LoginServer> mLoginServer;
		Poco::DateTime mCreated;
		std::string mGroupAlias;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_RECOVER_LOGIN_KEY_PAIR_H