#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_SESSION_MANAGER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_SESSION_MANAGER_H

#include "Poco/AccessExpireCache.h"
#include "Poco/JWT/Token.h"

#include "model/Session.h"

class SessionManager
{
public:
	~SessionManager();

	static SessionManager* getInstance();

	Poco::SharedPtr<Session> getSession(const Poco::JWT::Token* jwtToken);
protected:
	SessionManager();
	
	Poco::AccessExpireCache<std::string, Session> mActiveSessions;
};


#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_SESSION_MANAGER_H