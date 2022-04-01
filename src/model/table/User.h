#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H

#include "BaseTable.h"
#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/crypto/KeyPairEd25519.h"


#define USER_TABLE_SCHEMA												\
	"`id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,"						\
	"`name` varchar(150) NOT NULL,"										\
	"`password` bigint unsigned NOT NULL,"								\
	"`public_key` binary(32) DEFAULT NULL,"							    \
	"`encrypted_private_key` binary(80) DEFAULT NULL,"					\
	"`created` datetime NOT NULL DEFAULT current_timestamp(),"			\
	"PRIMARY KEY(`id`),"												\
	"UNIQUE KEY `name` (`name`)"

#define USER_TABLE_LAST_SCHEMA_VERSION 1

namespace model {
	namespace table {

		typedef std::tuple<uint64_t, std::string, KeyHashed, li::sql_blob, li::sql_blob> UserTuple;

		class User: public BaseTable
		{
		public:
			User();
			User(const std::string& name, KeyHashed password, const unsigned char* publicKey, const MemoryBin* encyptedPrivateKey);
			User(UserTuple data);
			~User();
			
			static std::unique_ptr<User> load(const std::string& name);

			inline const std::string& getName() const { return mName; }
			inline KeyHashed getPassword() const { return mPassword; }
			inline const li::sql_blob& getPublicKey() const { return mPublicKey; }

			inline void setName(const std::string& name) { mName = name; }
			inline void setPassword(KeyHashed password) { mPassword = password; }

			// insert or update if id is != 0
			uint64_t save();

			const char* tableName() const { return getTableName(); }
			static const char* getTableName() { return "user"; }

		protected:
			std::string mName;
			KeyHashed   mPassword;
			li::sql_blob mPublicKey;
			li::sql_blob mEncryptedPrivateKey;
		};
	}
}



#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H