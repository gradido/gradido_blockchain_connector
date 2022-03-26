
#include "VersionsManager.h"

#include "ServerConfig.h"

// tables
#include "Migrations.h"
#include "User.h"
#include "UserBackup.h"

using namespace li;

namespace model {
	namespace table {

		VersionsManager* VersionsManager::getInstance()
		{
			static VersionsManager one;
			return &one;
		}

		void VersionsManager::migrate()
		{
			auto schema = MIGRATE_TABLE_SCHEMA(*ServerConfig::g_Mysql, "migration");
			auto migrations = schema.connect();
			migrations.create_table_if_not_exists();
			
			bool userTable = false;
			bool userBackupTable = false;

			migrations.forall([&](auto u) {
				if (u.table_name == "user") {
					if (u.version < USER_TABLE_LAST_SCHEMA_VERSION) {
						throw std::runtime_error("missing migration implementation for new user table version");
					}
					userTable = true;
				}
				else if (u.table_name == "user_backup") {
					if (u.version < USER_BACKUP_TABLE_LAST_SCHEMA_VERSION) {
						throw std::runtime_error("missing migration implementation for new user backup table version");
					}
					userBackupTable = true;
				}
			});
			if (!userTable) {
				auto userSchema = USER_TABLE_LAST_SCHEMA(*ServerConfig::g_Mysql, "user");
				auto users = userSchema.connect();
				users.create_table_if_not_exists();
				migrations.insert(s::table_name = "user", s::version = USER_TABLE_LAST_SCHEMA_VERSION);
			}
			if (!userBackupTable) {
				auto userSchema = USER_BACKUP_TABLE_LAST_SCHEMA(*ServerConfig::g_Mysql, "user_backup");
				auto userBackups = userSchema.connect();
				userBackups.create_table_if_not_exists();
				migrations.insert(s::table_name = "user_backup", s::version = USER_BACKUP_TABLE_LAST_SCHEMA_VERSION);
			}

		}
	}
}