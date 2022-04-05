#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H

#include "BaseTable.h"
#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/crypto/KeyPairEd25519.h"


#define USER_TABLE_SCHEMA												\
	"`id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,"						\
	"`name` varchar(150) NOT NULL,"										\
    "`group_id` bigint UNSIGNED NOT NULL,"								\
	"`password` bigint unsigned NOT NULL,"								\
	"`public_key` binary(32) DEFAULT NULL,"							    \
	"`encrypted_private_key` binary(80) DEFAULT NULL,"					\
	"`created` datetime NOT NULL DEFAULT current_timestamp(),"			\
	"PRIMARY KEY(`id`),"												\
	"UNIQUE KEY `group_name` (`name`, `group_id`)"

#define USER_TABLE_LAST_SCHEMA_VERSION 1

namespace model {
	namespace table {

		typedef Poco::Tuple<uint64_t, std::string, uint64_t, KeyHashed, std::string, std::string> UserTuple;

		class User: public BaseTable
		{
		public:
			User();
			User(const std::string& name, uint64_t groupId, KeyHashed password, const unsigned char* publicKey, const MemoryBin* encryptedPrivateKey);
			User(const UserTuple& data);
			~User();
			
			static std::unique_ptr<User> load(const std::string& name);

			inline const std::string& getName() const { return mName; }
			inline uint64_t getGroupId() const { return mGroupId; }
			inline KeyHashed getPassword() const { return mPassword; }
			inline const std::string& getPublicKey() const { return mPublicKey; }

			inline void setName(const std::string& name) { mName = name; }
			inline void setGroupId(uint64_t groupId) { mGroupId = groupId; }
			inline void setPassword(KeyHashed password) { mPassword = password; }

			// insert or update if id is != 0
			void save(Poco::Data::Session& dbSession);

			const char* tableName() const { return getTableName(); }
			static const char* getTableName() { return "user"; }
			int getLastSchemaVersion() const { return USER_TABLE_LAST_SCHEMA_VERSION; }
			const char* getSchema() const { return USER_TABLE_SCHEMA; }

		protected:
			std::string mName;
			uint64_t	mGroupId;
			KeyHashed   mPassword;
			std::string mPublicKey;
			std::string mEncryptedPrivateKey;
		};
	}
}



#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H