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

	std::string login(const std::string& username, const std::string& password, const std::string& groupAlias, const std::string& clientIp);
	Poco::SharedPtr<model::Session> getSession(const std::string& serializedJwtToken, const std::string& clientIp);
	Poco::SharedPtr<model::Session> getSession(const std::string& serializedJwtToken, const std::string& username, const std::string& clientIp);

protected:
	SessionManager();
	
	Poco::AccessExpireCache<std::string, model::Session> mActiveSessions;
};


// ################## Exceptions ##############################
class LoginException : public GradidoBlockchainException
{
public:
	explicit LoginException(const char* what, const std::string& username, const std::string& clientIp) noexcept;

	std::string getFullString() const;
protected:
	std::string mUsername;
	std::string mClientIp;
};

class SessionException : public GradidoBlockchainException
{
public:
	explicit SessionException(const char* what, const std::string& username) noexcept;
	std::string getFullString() const;

protected:
	std::string mUsername;
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_SESSION_MANAGER_H