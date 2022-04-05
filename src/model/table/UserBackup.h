#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_BACKUP_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_BACKUP_H

#include "BaseTable.h"


#define USER_BACKUP_TABLE_SCHEMA												\
	"`id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,"						\
	"`user_id` bigint UNSIGNED NOT NULL,"								\
	"`key_user_id` bigint UNSIGNED NOT NULL,"							\
	"`encrypted_passphrase` varbinary(255) DEFAULT NULL,"				\
	"`created` datetime NOT NULL DEFAULT current_timestamp(),"			\
	"PRIMARY KEY(`id`)"												


#define USER_BACKUP_TABLE_LAST_SCHEMA_VERSION 1

namespace model {
	namespace table {

		typedef Poco::Tuple<uint64_t, uint64_t, uint64_t, std::string> UserBackupTuple;

		class UserBackup : public BaseTable
		{
		public:
			UserBackup();
			UserBackup(uint64_t userId, uint64_t keyUserId, const MemoryBin* encryptedPassphrase);
			UserBackup(const UserBackupTuple& data);
			~UserBackup();

			const char* tableName() const { return getTableName(); }
			static const char* getTableName() { return "user_backup"; }
			int getLastSchemaVersion() const { return USER_BACKUP_TABLE_LAST_SCHEMA_VERSION; }
			const char* getSchema() const { return USER_BACKUP_TABLE_SCHEMA; }

			inline uint64_t getUserId() const { return mUserId; }
			inline uint64_t getKeyUserId() const { return mKeyUserId; }
			inline const std::string& getEncryptedPassphrase() const { return mEncryptedPassphrase; }

			inline void setUserId(uint64_t userId) { mUserId = userId; }
			inline void setKeyUserId(uint64_t keyUserId) { mKeyUserId = keyUserId; }
			inline void setEncryptedPassphrase(const MemoryBin* encryptedPassphrase) { 
				mEncryptedPassphrase = *encryptedPassphrase->copyAsString().get();
			}

			void save(Poco::Data::Session& dbSession);

		protected:
			uint64_t mUserId;
			uint64_t mKeyUserId;
			std::string mEncryptedPassphrase;

		};

	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_BACKUP_H
