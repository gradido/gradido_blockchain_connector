#include "User.h"
#include "ConnectionManager.h"
#include <sstream>

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {		

		User::User()
			: mGroupId(0), mPassword(0)
		{
			
		}

		User::User(const std::string& name, uint64_t groupId, KeyHashed password, const unsigned char* publicKey, const MemoryBin* encyptedPrivateKey)
			: mName(name), mGroupId(groupId), mPassword(password), mPublicKey((const char*)publicKey, KeyPairEd25519::getPublicKeySize()), mEncryptedPrivateKey(*encyptedPrivateKey->copyAsString().get())
		{
			
		}

		User::User(UserTuple data)
			: BaseTable(std::get<0>(data)), mName(std::get<1>(data)), mGroupId(std::get<2>(data)), mPassword(std::get<3>(data)), 
			mPublicKey(std::get<4>(data)), mEncryptedPrivateKey(std::get<5>(data))
		{

		}

		User::~User()
		{
		}

		std::unique_ptr<User> User::load(const std::string& name)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);

			UserTuple userTuple;

			select << "SELECT id, name, group_id, password, public_key, encrypted_private_key "
				<< " from " << getTableName()
				<< " where name LIKE ?",
				into(userTuple), useRef(name);

			if (!select.execute()) {
				throw RowNotFoundException("couldn't load user", getTableName(), "where name LIKE " + name);
			}
			auto user = std::make_unique<User>(userTuple);
			return std::move(user);
		}
		
		uint64_t User::save()
		{
			auto dbConnection = ServerConfig::g_Mysql->connect();
			std::stringstream sqlQuery;
			sqlQuery << "INSERT INTO " << getTableName() << "(name, group_id, password, public_key, encrypted_private_key) VALUES(?,?,?,?)";
			
			auto preparedStatement = dbConnection.prepare(sqlQuery.str());
			auto result = preparedStatement(mName, mGroupId, mPassword, mPublicKey, mEncryptedPrivateKey);
			return result.last_insert_id();
		}

	}
}