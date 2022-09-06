
#include "VersionsManager.h"
#include "ConnectionManager.h"

#include "ServerConfig.h"

// tables
#include "Migration.h"
#include "User.h"
#include "UserBackup.h"
#include "Group.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {

		VersionsManager::VersionsManager()
		{
		}

		VersionsManager* VersionsManager::getInstance()
		{
			static VersionsManager one;
			return &one;
		}

		void VersionsManager::migrate()
		{	
			auto cm = ConnectionManager::getInstance();
			auto dbSession = cm->getConnection();

			createTableIfNotExist(dbSession, Migration::getTableName(), cm->isSqlite() ? MIGRATION_TABLE_SCHEMA_SQLITE : MIGRATION_TABLE_SCHEMA);
			
			bool userTable = false;
			bool userBackupTable = false;
			bool groupTable = false;
			std::unordered_set<std::string> existingTables;
			std::vector<std::string> requiredTables = { User::getTableName(), Group::getTableName(), UserBackup::getTableName() };

			auto migrations = Migration::loadAll();

			std::for_each(migrations.begin(), migrations.end(), [&](model::table::MigrationTuple& tuple) {
				Migration migration(tuple);
				auto tableName = migration.getTableNameValue();
				auto tableBase = factoryTable(tableName);				
				if (migration.getVersion() < tableBase->getLastSchemaVersion()) {
					throw MissingMigrationException("migration missing", tableName, migration.getVersion());
				}
				existingTables.insert(tableName);
			});
			
			std::for_each(requiredTables.begin(), requiredTables.end(), [&](const std::string& tableName) {
				if (existingTables.find(tableName) == existingTables.end()) {
					auto tableBase = factoryTable(tableName);
					createTableIfNotExist(dbSession, tableBase->tableName(), tableBase->getSchema());
					auto migration = std::make_unique<Migration>(tableBase->tableName(), tableBase->getLastSchemaVersion());
					migration->save(dbSession);
				}
			});			
		}

		void VersionsManager::createTableIfNotExist(Poco::Data::Session& dbSession, const std::string& tablename, const std::string& tableDefinition)
		{
			Poco::Data::Statement createTable(dbSession);
			createTable << "CREATE TABLE IF NOT EXISTS `" + tablename + "` (" + tableDefinition + ")";
			if (!ConnectionManager::getInstance()->isSqlite()) {
				createTable << "ENGINE = InnoDB DEFAULT CHARSET = utf8mb4";
			}
			createTable << ";", now;
		}

		std::unique_ptr<BaseTable> VersionsManager::factoryTable(const std::string& tableName)
		{
			if (tableName == User::getTableName()) {
				return std::make_unique<User>();
			}
			else if (tableName == Group::getTableName()) {
				return std::make_unique<Group>();
			}
			else if (tableName == UserBackup::getTableName()) {
				return std::make_unique<UserBackup>();
			}
			throw UnknownTableNameException("unknown tableName", tableName);
		}


		// ************************** Exceptions *******************************
		UnknownTableNameException::UnknownTableNameException(const char* what, const std::string& tableName) noexcept
			: GradidoBlockchainException(what), mTableName(tableName)
		{

		}

		std::string UnknownTableNameException::getFullString() const
		{
			std::string result(what());
			result += ", table name: " + mTableName;
			return result;
		}

		MissingMigrationException::MissingMigrationException(const char* what, const std::string& tableName, uint32_t version) noexcept
			: GradidoBlockchainException(what), mTableName(tableName), mVersion(version)
		{

		}

		std::string MissingMigrationException::getFullString() const
		{
			std::string result(what());
			result += ", table name: " + mTableName;
			result += ", version: " + std::to_string(mVersion);
			return result;
		}
	}
}