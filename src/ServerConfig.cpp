#include "ServerConfig.h"
#include "ConnectionManager.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "sodium.h"


#include "Poco/Net/SSLManager.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/RejectCertificateHandler.h"
#include "Poco/Net/DNS.h"
#include "Poco/SharedPtr.h"

#include "Poco/Mutex.h"
#include "Poco/Path.h"
#include "Poco/FileStream.h"
#include "Poco/LocalDateTime.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Environment.h"



using Poco::Net::SSLManager;
using Poco::Net::Context;
using Poco::Net::KeyConsoleHandler;
using Poco::Net::PrivateKeyPassphraseHandler;
using Poco::Net::InvalidCertificateHandler;
using Poco::Net::RejectCertificateHandler;
using Poco::SharedPtr;

namespace ServerConfig {
	Context::Ptr g_SSL_CLient_Context = nullptr;
	Poco::URI    g_GradidoNodeUri;
	std::string g_versionString = "";
	ServerSetupType g_ServerSetupType = SERVER_TYPE_PRODUCTION;
	MemoryBin*  g_CryptoAppSecret = nullptr;
	MemoryBin* g_ApolloJwtPublicKey = nullptr;
	MemoryBin* g_JwtPrivateKey = nullptr;
	std::string g_JwtVerifySecret;
	Poco::Timespan g_SessionValidDuration = Poco::Timespan(600, 0);

	//li::mysql_database* g_Mysql = nullptr;

	ServerSetupType getServerSetupTypeFromString(const std::string& serverSetupTypeString) {
		if ("test" == serverSetupTypeString) {
			return SERVER_TYPE_TEST;
		}
		if ("staging" == serverSetupTypeString) {
			return SERVER_TYPE_STAGING;
		}
		if ("production" == serverSetupTypeString) {
			return SERVER_TYPE_PRODUCTION;
		}
		return SERVER_TYPE_PRODUCTION;
	}


	bool initServerCrypto(const MapEnvironmentToConfig& cfg)
	{
		auto serverKey = cfg.getString("crypto.server_key", "a51ef8ac7ef1abf162fb7a65261acd7a");
		unsigned char key[crypto_shorthash_KEYBYTES];
		size_t realBinSize = 0;
		if (sodium_hex2bin(key, crypto_shorthash_KEYBYTES, serverKey.data(), serverKey.size(), nullptr, &realBinSize, nullptr)) {
			printf("[%s] serverKey isn't valid hex: %s\n", __FUNCTION__, serverKey.data());
			return false;
		}
		if (realBinSize != crypto_shorthash_KEYBYTES) {
			printf("[%s] serverKey hasn't valid size, expecting: %u, get: %lu\n",
				__FUNCTION__, crypto_shorthash_KEYBYTES, realBinSize);
			return false;
		}
		
		auto serverSetupTypeString = cfg.getString("server_setup_type", "");
		g_ServerSetupType = getServerSetupTypeFromString(serverSetupTypeString);

		
		// app secret for encrypt user private keys
		// TODO: encrypt with server admin key
		auto app_secret_string = cfg.getString("crypto.app_secret", "21ffbbc616fe");
		if ("" != app_secret_string) {
			g_CryptoAppSecret = DataTypeConverter::hexToBin(app_secret_string);
		}
		
		// load keys for jwt
		auto apolloPublicKeyString = cfg.getString("crypto.jwt.apollo_public", "");
		if (apolloPublicKeyString.size()) {
			g_ApolloJwtPublicKey = DataTypeConverter::hexToBin(apolloPublicKeyString);
		}
		
		auto privateKeyString = cfg.getString("crypto.jwt.private", "");
		if (privateKeyString.size()) {
			g_JwtPrivateKey = DataTypeConverter::hexToBin(privateKeyString);
		}
		g_JwtVerifySecret = cfg.getString("verify.jwt", "");

		g_SessionValidDuration = Poco::Timespan(cfg.getInt("session.duration_seconds", 600), 0);

		return true;
	}

	

	bool initSSLClientContext()
	{
		SharedPtr<InvalidCertificateHandler> pCert = new RejectCertificateHandler(false); // reject invalid certificates
		/*
		Context(Usage usage,
		const std::string& certificateNameOrPath,
		VerificationMode verMode = VERIFY_RELAXED,
		int options = OPT_DEFAULTS,
		const std::string& certificateStoreName = CERT_STORE_MY);
		*/
#ifdef POCO_NETSSL_WIN
		try {
			g_SSL_CLient_Context = new Context(Context::CLIENT_USE, "cacert.pem", Context::VERIFY_RELAXED, Context::OPT_DEFAULTS);
		} catch(Poco::Exception& ex) {
			printf("[ServerConfig::initSSLClientContext] error init ssl context, maybe no cacert.pem found?\nPlease make sure you have cacert.pem (CA/root certificates) next to binary from https://curl.haxx.se/docs/caextract.html\n");
			return false;
		}
#else
		try {
			g_SSL_CLient_Context = new Context(Context::CLIENT_USE, "", "", Poco::Path::home() + ".gradido/cacert.pem", Context::VERIFY_RELAXED, 9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		} catch(Poco::Exception& ex) {
			printf("[ServerConfig::initSSLClientContext] error init ssl context, maybe no cacert.pem found?\nPlease make sure you have cacert.pem (CA/root certificates) inside ~/.gradido from https://curl.haxx.se/docs/caextract.html\n");
			return false;
		}
#endif
		SSLManager::instance().initializeClient(0, pCert, g_SSL_CLient_Context);


		return true;
	}

	bool initMysql(const MapEnvironmentToConfig& cfg)
	{
		/*
		db.host = 192.168.178.225
		db.name = gradido_community
		db.user = gradido_community
		db.password = asjeuKJs783laskawk
		db.port = 3306
		db.charset = utf8mb4_general_ci
		db.max_connections = 2000
		*/
		ConnectionManager::getInstance()->setConnectionsFromConfig(cfg);
		// Declare a mysql database.
		/*g_Mysql = new li::mysql_database(
			s::host = cfg.getString("db.host", "127.0.0.1"), // Hostname or ip of the database server
			s::database = cfg.getString("db.name", "blockchain_connector"),  // Database name
			s::user = cfg.getString("db.name", "root"), // Username
			s::password = cfg.getString("db.password", ""), // Password
			s::port = cfg.getInt("db.port", 3306), // Port
			s::charset = cfg.getString("db.charset", "utf8mb4_general_ci"), // Charset
			// Only for synchronous connection, specify the maximum number of SQL connections
			s::max_sync_connections = cfg.getInt("db.max_connections", 2000)
		);*/
		return true;		
	}

	void unload() 
	{
		auto mm = MemoryManager::getInstance();

		if (g_CryptoAppSecret) {
			mm->releaseMemory(g_CryptoAppSecret);
			g_CryptoAppSecret = nullptr;
		}
		/*if (g_Mysq) {
			delete g_Mysql;
			g_Mysql = nullptr;
		}*/
		
		if (g_ApolloJwtPublicKey) {
			mm->releaseMemory(g_ApolloJwtPublicKey);
			g_ApolloJwtPublicKey = nullptr;
		}
		if (g_JwtPrivateKey) {
			mm->releaseMemory(g_JwtPrivateKey);
			g_JwtPrivateKey = nullptr;
		}
	}

	void writeToFile(std::istream& datas, std::string fileName)
	{
		static Poco::Mutex mutex;

		mutex.lock();

		Poco::FileOutputStream file(fileName, std::ios::out | std::ios::app);

		if (!file.good()) {
			printf("[ServerConfig::writeToFile] error creating file with name: %s\n", fileName.data());
			mutex.unlock();
			return;
		}

		Poco::LocalDateTime now;

		std::string dateTimeStr = Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::ISO8601_FORMAT);
		file << dateTimeStr << std::endl;

		for (std::string line; std::getline(datas, line); ) {
			file << line << std::endl;
		}
		file << std::endl;
		file.close();
		mutex.unlock();
	}
}
