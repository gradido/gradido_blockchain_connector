#ifndef __GRADIDO_LOGIN_SERVER_SERVER_CONFIG__
#define __GRADIDO_LOGIN_SERVER_SERVER_CONFIG__


#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Net/Context.h"
#include "Poco/Types.h"
#include "Poco/Util/Timer.h"

#include "gradido_blockchain/MemoryManager.h"

#include "gradido_blockchain/http/IotaRequest.h"

#define DISABLE_EMAIL

namespace ServerConfig {

	
	enum ServerSetupType {
		SERVER_TYPE_TEST,
		SERVER_TYPE_STAGING,
		SERVER_TYPE_PRODUCTION
	};

	// used with bit-operators, so only use numbers with control exactly one bit (1,2,4,8,16...)
	enum AllowUnsecure {
		NOT_UNSECURE = 0,
		UNSECURE_PASSWORD_REQUESTS = 1,
		UNSECURE_AUTO_SIGN_TRANSACTIONS = 2,
		UNSECURE_CORS_ALL = 4,
		UNSECURE_ALLOW_ALL_PASSWORDS = 8
	};


	extern Poco::Net::Context::Ptr g_SSL_CLient_Context;
	extern IotaRequest* g_IotaRequestHandler;
	extern std::string g_versionString;
	extern ServerSetupType g_ServerSetupType;
	extern MemoryBin*  g_CryptoAppSecret;
	extern AllowUnsecure g_AllowUnsecureFlags;

	bool initServerCrypto(const Poco::Util::LayeredConfiguration& cfg);
	bool initSSLClientContext();
	bool initIota(const Poco::Util::LayeredConfiguration& cfg);

	void writeToFile(std::istream& datas, std::string fileName);

	void unload();
};


#endif //__GRADIDO_LOGIN_SERVER_SERVER_CONFIG__
