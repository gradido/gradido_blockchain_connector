#include "SessionManager.h"
#include "ServerConfig.h"
#include "gradido_blockchain/http/JsonRequestHandlerJwt.h"

#include "Poco/JWT/Signer.h"

// TODO: Move into config
// 600000 = 10 min
SessionManager::SessionManager()
	: mActiveSessions(ServerConfig::g_SessionValidDuration.totalMilliseconds())
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

std::string SessionManager::login(const std::string& username, const std::string& password, const std::string& clientIp)
{
	auto session = mActiveSessions.get(username);
	if (session.isNull()) {
		session = new Session;
		session->loginOrCreate(username, password, clientIp);
	}
	if (session->getClientIp() != clientIp) {
		throw LoginException("invalid ip", username, clientIp);
	}

	Poco::Timestamp now;
	Poco::JWT::Token token;
	token.setType("JWT");
	token.setSubject("login");
	token.payload().set("name", username);
	token.setIssuedAt(now);
	token.setExpiration(now + ServerConfig::g_SessionValidDuration);

	Poco::JWT::Signer signer(ServerConfig::g_JwtVerifySecret);
	return std::move(signer.sign(token, Poco::JWT::Signer::ALGO_HS256));
}

Poco::SharedPtr<Session> SessionManager::getSession(const std::string& serializedJwtToken)
{
	try {
		Poco::JWT::Signer signer(ServerConfig::g_JwtVerifySecret);
		Poco::JWT::Token token = signer.verify(serializedJwtToken);
	}
	catch (Poco::Exception& ex) {
		// SignatureVerificationException
		printf("jwt error: %s\n", ex.displayText().data());
		throw JwtTokenException("invalid jwtToken", serializedJwtToken);
	}
	try {
		Poco::JWT::Token token(serializedJwtToken);
		auto payloadJson = token.payload();
		auto name = payloadJson.getValue<std::string>("name");

		auto session = mActiveSessions.get(name);
		if (session.isNull()) {
			throw SessionException("no session found", name);
		}
		return session;
	}
	catch (Poco::Exception& ex) {
		throw JwtTokenException("jwtToken not like expected", serializedJwtToken);
	}	
}


// ++++++++++++++++++++++++++ Login Exception ++++++++++++++++++++++++++++++++++++
LoginException::LoginException(const char* what, const std::string& username, const std::string& clientIp) noexcept
	: GradidoBlockchainException(what), mUsername(username), mClientIp(clientIp)
{

}

std::string LoginException::getFullString() const
{
	std::string result = what();
	result += ", user: " + mUsername + " with ip: " + mClientIp;
	return std::move(result);
}

// ++++++++++++++++++++++++++ Session Exception ++++++++++++++++++++++++++++++++++++
SessionException::SessionException(const char* what, const std::string& username) noexcept
	: GradidoBlockchainException(what), mUsername(username)
{

}

std::string SessionException::getFullString() const
{
	std::string result = what();
	result += ", user: " + mUsername;
	return std::move(result);
}