#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_MIGRATE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_MIGRATE_H

#include "BaseTable.h"

#define MIGRATION_TABLE_SCHEMA												\
	"`id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,"						\
	"`table_name` varchar(150) NOT NULL,"										\
    "`version` int UNSIGNED NOT NULL,"								\
	"PRIMARY KEY(`id`),"												\
	"UNIQUE KEY `table_name` (`table_name`)"

#define MIGRATION_TABLE_LAST_SCHEMA_VERSION 1

namespace model {
	namespace table {

		typedef Poco::Tuple<uint64_t, std::string, uint32_t> MigrationTuple;

		class Migration : public BaseTable
		{
		public:
			Migration();
			Migration(const std::string& tableName, int version);
			Migration(const MigrationTuple& tuple);

			static std::unique_ptr<Migration> load(const std::string& tableName);
			static std::vector<MigrationTuple> loadAll();
			void save(Poco::Data::Session& dbSession);

			inline int getVersion() const { return mVersion; }
			inline const std::string& getTableNameValue() const { return mTableName; }

			const char* tableName() const { return getTableName(); }			
			static const char* getTableName() { return "migration"; }
			int getLastSchemaVersion() const { return MIGRATION_TABLE_LAST_SCHEMA_VERSION; }
			const char* getSchema() const { return MIGRATION_TABLE_SCHEMA; }

		private:
			std::string mTableName;
			int			mVersion;
		};
	}
}



#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_MIGRATE_H
