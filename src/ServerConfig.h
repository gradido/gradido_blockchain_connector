#ifndef __GRADIDO_LOGIN_SERVER_SERVER_CONFIG__
#define __GRADIDO_LOGIN_SERVER_SERVER_CONFIG__

// MySQL header.
// Lithium need 'and' and 'or' defined
/*#ifndef and 
#define and &&
#endif

#ifndef or 
#define or ||
#endif
*/
//#include "lithium_mysql.hh"

#include "Poco/Net/Context.h"
#include "Poco/Types.h"
#include "Poco/Timespan.h"
#include "Poco/Util/Timer.h"
#include "Poco/URI.h"

#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/http/IotaRequest.h"
#include "gradido_blockchain/lib/MapEnvironmentToConfig.h"

#define DISABLE_EMAIL

namespace ServerConfig {

	
	enum ServerSetupType {
		SERVER_TYPE_TEST,
		SERVER_TYPE_STAGING,
		SERVER_TYPE_PRODUCTION
	};

	extern Poco::Net::Context::Ptr g_SSL_CLient_Context;
	extern Poco::URI    g_GradidoNodeUri;
	extern std::string g_versionString;
	extern ServerSetupType g_ServerSetupType;
	extern MemoryBin*  g_CryptoAppSecret;
	extern MemoryBin* g_ApolloJwtPublicKey;
	extern MemoryBin* g_JwtPrivateKey;
	extern std::string g_JwtVerifySecret;
	extern Poco::Timespan g_SessionValidDuration;
	
	//extern li::mysql_database* g_Mysql;

	bool initServerCrypto(const MapEnvironmentToConfig& cfg);
	bool initSSLClientContext();
	bool initMysql(const MapEnvironmentToConfig& cfg);

	void writeToFile(std::istream& datas, std::string fileName);

	void unload();
};


#endif //__GRADIDO_LOGIN_SERVER_SERVER_CONFIG__
