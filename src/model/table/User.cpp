#include "User.h"

#include <sstream>

namespace model {
	namespace table {
		User::User()
			: mPassword(0)
		{
			
		}

		User::User(const std::string& name, KeyHashed password, const unsigned char* publicKey, const MemoryBin* encyptedPrivateKey)
			: mName(name), mPassword(password), mPublicKey((const char*)publicKey, KeyPairEd25519::getPublicKeySize()), mEncryptedPrivateKey(*encyptedPrivateKey->copyAsString().get())
		{
			
		}

		User::User(UserTuple data)
			: BaseTable(std::get<0>(data)), mName(std::get<1>(data)), mPassword(std::get<2>(data)), 
			mPublicKey(std::get<3>(data)), mEncryptedPrivateKey(std::get<4>(data))
		{

		}

		User::~User()
		{
		}

		std::unique_ptr<User> User::load(const std::string& name)
		{
			auto dbConnection = ServerConfig::g_Mysql->connect();
			std::stringstream sqlQuery;
			sqlQuery << "SELECT id, name, password, public_key, encrypted_private_key from " << getTableName() << " where name LIKE ?";
			auto preparedStatement = dbConnection.prepare(sqlQuery.str());
			auto result = preparedStatement(name).read_optional<UserTuple>();
			auto user = std::make_unique<User>(result.value());
			return std::move(user);
		}
		
		uint64_t User::save()
		{
			auto dbConnection = ServerConfig::g_Mysql->connect();
			std::stringstream sqlQuery;
			sqlQuery << "INSERT INTO " << getTableName() << "(name, password, public_key, encrypted_private_key) VALUES(?,?,?,?)";
			
			auto preparedStatement = dbConnection.prepare(sqlQuery.str());
			auto result = preparedStatement(mName, mPassword, mPublicKey, mEncryptedPrivateKey);
			return result.last_insert_id();
		}



	}
}