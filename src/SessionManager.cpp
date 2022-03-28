#include "SessionManager.h"

SessionManager::SessionManager()
{

}

SessionManager::~SessionManager()
{

}

SessionManager* SessionManager::getInstance()
{
	static SessionManager one;
	return &one;
}

Poco::SharedPtr<Session> SessionManager::getSession(const Poco::JWT::Token* jwtToken)
{
	auto payloadJson = jwtToken->payload();
	auto name = payloadJson.getValue<std::string>("name");
	auto session = mActiveSessions.get(name);
	if (session.isNull()) {
		session = new Session;
		printf("jwt token as string: %s\n", jwtToken->toString().data());
		//session->loginOrCreate(name, )
	}
	return session;
}