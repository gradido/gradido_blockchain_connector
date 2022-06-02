#include "ConnectionManager.h"
#include <sstream>

ConnectionManager* ConnectionManager::getInstance()
{
	static ConnectionManager only;
	return &only;
}


ConnectionManager::ConnectionManager()
{

}

ConnectionManager::~ConnectionManager()
{

}

bool ConnectionManager::setConnectionsFromConfig(const Poco::Util::LayeredConfiguration& config)
{
	/*
	phpServer.url = 127.0.0.1:80/gradido_php
	phpServer.db.host = localhost
	phpServer.db.name = cake_gradido_node
	phpServer.db.user = root
	phpServer.db.password =
	phpServer.db.port = 3306

	loginServer.url =
	loginServer.db.host = localhost
	loginServer.db.name = gradido_login
	loginServer.db.user = gradido_login
	loginServer.db.password = hj2-sk28sKsj8(u_ske
	loginServer.db.port = 3306
	*/

	/*
	connectionString example: host=localhost;port=3306;db=mydb;user=alice;password=s3cr3t;compress=true;auto-reconnect=true
	*/
	
	std::stringstream dbConfig;
	dbConfig << "host=" << config.getString("db.host", "localhost") << ";";
	dbConfig << "port=" << config.getInt("db.port", 3306) << ";";
	std::string dbName = config.getString("db.name", "blockchain_connector");
	dbConfig << "db=" << dbName << ";";
	dbConfig << "user=" << config.getString("db.user", "root") << ";";
	dbConfig << "password=" << config.getString("db.password", "") << ";";
	dbConfig << "character-set=utf8mb4;";
	//dbConfig << "auto-reconnect=true";
	//std::clog << "try connect with: " << dbConfig.str() << std::endl;
	Poco::Data::MySQL::Connector::registerConnector();
	setConnection(dbConfig.str());

	return true;

}

Poco::Data::Session ConnectionManager::getConnection() 
{
	Poco::ScopedLock<Poco::FastMutex> _lock(mWorkingMutex);
	return mSessionPools.getPool(mSessionPoolNames).get();
}