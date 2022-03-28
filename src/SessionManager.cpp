#include "SessionManager.h"
#include "gradido_blockchain/http/JsonRequestHandlerJwt.h"

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
	try {
		auto payloadJson = jwtToken->payload();
		auto data = payloadJson.getObject("data");
		auto publicData = data->getObject("public");
		auto name = publicData->getValue<std::string>("name");

		auto session = mActiveSessions.get(name);
		if (session.isNull()) {
			session = new Session;
			if (data->get("encData").isString()) {
				printf("encData: %s\n", data->getValue<std::string>("encData").data());
			}
			else {
				auto encData = data->getObject("encData");
				std::stringstream ss;
				Poco::JSON::Stringifier::stringify(encData, ss);
				printf("encData: %s\n", ss.str().data());
			}
			//
			printf("jwt type: %s, algo: %s\n", jwtToken->getType().data(), jwtToken->getAlgorithm().data());
			
			
			//session->loginOrCreate(name, )
		}
		return session;
	}
	catch (Poco::Exception& ex) {
		throw JwtTokenException("jwtToken not like expected", jwtToken);
	}	
}