#ifndef GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_CONNECTION_MANAGER_INCLUDE
#define GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_CONNECTION_MANAGER_INCLUDE

#include <string>

#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Data/SessionPoolContainer.h"
#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Exception.h"



class ConnectionManager
{
public: 
	~ConnectionManager();

	static ConnectionManager* getInstance();

	bool setConnectionsFromConfig(const Poco::Util::LayeredConfiguration& config);

	//!  \param connectionString example: host=localhost;port=3306;db=mydb;user=alice;password=s3cr3t;compress=true;auto-reconnect=true
	inline void setConnection(std::string connectionString) {
		mSessionPoolNames = Poco::Data::Session::uri(Poco::Data::MySQL::Connector::KEY, connectionString);
		mSessionPools.add(Poco::Data::MySQL::Connector::KEY, connectionString, 1, 16);
	}

	//! \brief return connection from pool, check if connected in if not, call reconnect on it
	//! 
	//! In the past I used auto-reconnect but it didn't work everytime as expectet
	Poco::Data::Session getConnection();

protected:
	ConnectionManager();

private:
	std::string mSessionPoolNames;
	Poco::Data::SessionPoolContainer mSessionPools;
	Poco::FastMutex  mWorkingMutex;

};

#endif //GRADIDO_LOGIN_SERVER_SINGLETON_MANAGER_CONNECTION_MANAGER_INCLUDE