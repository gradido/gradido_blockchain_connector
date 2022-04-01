
#include "VersionsManager.h"

#include "ServerConfig.h"

// tables
#include "Migrations.h"
#include "User.h"
#include "UserBackup.h"

//using namespace li;

namespace model {
	namespace table {

		VersionsManager* VersionsManager::getInstance()
		{
			static VersionsManager one;
			return &one;
		}

		void VersionsManager::migrate()
		{			
			auto dbConnection = ServerConfig::g_Mysql->connect();
			auto schema = MIGRATE_TABLE_SCHEMA(*ServerConfig::g_Mysql, "migration");
			auto migrations = schema.connect();
			migrations.create_table_if_not_exists();
			
			bool userTable = false;
			bool userBackupTable = false;

			auto result = dbConnection("SELECT table_name, version from migration");
			
			result.map([&](std::string table_name, int version) {
				if (table_name == "user") {
					if (version < USER_TABLE_LAST_SCHEMA_VERSION) {
						throw std::runtime_error("missing migration implementation for new user table version");
					}
					userTable = true;
				}
				else if (table_name == "user_backup") {
					if (version < USER_BACKUP_TABLE_LAST_SCHEMA_VERSION) {
						throw std::runtime_error("missing migration implementation for new user backup table version");
					}
					userBackupTable = true;
				}
			});

			if (!userTable) {
				createTableIfNotExist(User::getTableName(), USER_TABLE_SCHEMA);
				migrations.insert(s::table_name = "user", s::version = USER_TABLE_LAST_SCHEMA_VERSION);
			}
			if (!userBackupTable) {
				createTableIfNotExist(UserBackup::getTableName(), USER_BACKUP_TABLE_SCHEMA);
				migrations.insert(s::table_name = "user_backup", s::version = USER_BACKUP_TABLE_LAST_SCHEMA_VERSION);
			}
			
		}

		void VersionsManager::createTableIfNotExist(const std::string& tablename, const std::string& tableDefinition)
		{
			auto dbConnection = ServerConfig::g_Mysql->connect();
			std::string sqlQuery = "CREATE TABLE IF NOT EXISTS `" + tablename + "` (" + tableDefinition + ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;";
			dbConnection(sqlQuery);
		}
	}
}