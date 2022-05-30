#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_RECOVER_LOGIN_KEY_PAIR_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_RECOVER_LOGIN_KEY_PAIR_H

#include "CPUTask.h"
#include "../model/import/LoginServer.h"
#include "Poco/AutoPtr.h"

namespace task
{
	class RecoverLoginKeyPair : public CPUTask
	{
	public:
		//! \param passphrase move string here
		RecoverLoginKeyPair(uint64_t id, std::string passphrase, Poco::AutoPtr<model::import::LoginServer> loginServer);
		~RecoverLoginKeyPair();

		const char* getResourceType() const { return "RecoverLoginKeyPair"; };
		int run();
	protected:
		uint64_t mId;
		std::string mPassphrase;
		Poco::AutoPtr<model::import::LoginServer> mLoginServer;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_RECOVER_LOGIN_KEY_PAIR_H