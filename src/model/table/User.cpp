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

		std::unordered_map<std::string, std::string> User::loadUserNamesForPubkeys(const std::vector<std::string>& pubkeysHex)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);

			typedef Poco::Tuple<std::string, std::string> NamePubkeyHex;
			std::vector<NamePubkeyHex> namePubkeysHex;

			select << "SELECT LOWER(HEX(public_key)), name "
				<< " from " << model::table::User::getTableName()
				<< " where hex(public_key) IN ("; 
			for (int i = 0; i < pubkeysHex.size(); i++) {
				if (i > 0) select << ",";
				select << "?";
			}
			select << ") ";
			select, into(namePubkeysHex);
			for (auto it = pubkeysHex.begin(); it != pubkeysHex.end(); it++) {
				select, useRef(*it);
			}
			//printf("mysql: %s\n", select.toString().data());
			select.execute();
			std::unordered_map<std::string, std::string> pubkeyHexNamesMap;
			for (auto it = namePubkeysHex.begin(); it != namePubkeysHex.end(); it++)
			{
				pubkeyHexNamesMap.insert({ it->get<0>(), it->get<1>() });
			}
			return std::move(pubkeyHexNamesMap);
		}


		
		void User::save(Poco::Data::Session& dbSession)
		{
			Poco::Data::Statement insert(dbSession);

			insert << "INSERT INTO " << getTableName() << "(name, group_id, password, public_key, encrypted_private_key) VALUES(?,?,?,?,?);",
				use(mName), use(mGroupId), use(mPassword), use(mPublicKey), use(mEncryptedPrivateKey), now;
		}

	}
}