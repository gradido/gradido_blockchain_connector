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

		typedef std::tuple<uint64_t, uint64_t, uint64_t, li::sql_blob> UserBackupTuple;

		class UserBackup : public BaseTable
		{
		public:
			UserBackup();
			UserBackup(uint64_t userId, uint64_t keyUserId, const MemoryBin* encryptedPassphrase);
			UserBackup(UserBackupTuple data);
			~UserBackup();

			const char* tableName() const { return getTableName(); }
			static const char* getTableName() { return "user_backup"; }

			inline uint64_t getUserId() const { return mUserId; }
			inline uint64_t getKeyUserId() const { return mKeyUserId; }
			inline const li::sql_blob& getEncryptedPassphrase() const { return mEncryptedPassphrase; }

			inline void setUserId(uint64_t userId) { mUserId = userId; }
			inline void setKeyUserId(uint64_t keyUserId) { mKeyUserId = keyUserId; }
			inline void setEncryptedPassphrase(const MemoryBin* encryptedPassphrase) { 
				mEncryptedPassphrase = *encryptedPassphrase->copyAsString().get();
			}

			uint64_t save();

		protected:
			uint64_t mUserId;
			uint64_t mKeyUserId;
			li::sql_blob mEncryptedPassphrase;

		};

	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_BACKUP_H
