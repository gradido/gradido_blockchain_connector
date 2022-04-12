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

		User::User(const std::string& name, uint64_t groupId, KeyHashed password, const unsigned char* publicKey, const MemoryBin* encryptedPrivateKey)
			: mName(name), mGroupId(groupId), mPassword(password), 
			mPublicKey((const char*)publicKey, KeyPairEd25519::getPublicKeySize()),
			mEncryptedPrivateKey(*encryptedPrivateKey->copyAsString().get())
		{
			
		}

		User::User(const UserTuple& data)
			: BaseTable(data.get<0>()), mName(data.get<1>()), mGroupId(data.get<2>()), mPassword(data.get<3>()),
			mPublicKey(data.get<4>()), mEncryptedPrivateKey(data.get<5>())
		{

		}

		User::~User()
		{
		}

		std::unique_ptr<User> User::load(const std::string& name, int groupId)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);

			UserTuple userTuple;

			select << "SELECT id, name, group_id, password, public_key, encrypted_private_key "
				<< " from " << getTableName()
				<< " where name LIKE ? AND group_id = ?",
				into(userTuple), useRef(name), useRef(groupId);

			if (!select.execute()) {
				throw RowNotFoundException("couldn't load user", getTableName(), "where name LIKE " + name);
			}
			auto user = std::make_unique<User>(userTuple);
			return std::move(user);
		}
		
		void User::save(Poco::Data::Session& dbSession)
		{
			Poco::Data::Statement insert(dbSession);

			insert << "INSERT INTO " << getTableName() << "(name, group_id, password, public_key, encrypted_private_key) VALUES(?,?,?,?,?);",
				use(mName), use(mGroupId), use(mPassword), use(mPublicKey), use(mEncryptedPrivateKey), now;
		}

	}
}