#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_GROUP_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_GROUP_H

#include "BaseTable.h"
#include "../../ConnectionManager.h"


#define GROUP_TABLE_SCHEMA												\
	"`id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,"						\
	"`name` varchar(150) NOT NULL,"										\
	"`groupAlias` varchar(150) NOT NULL,"								    \
	"`description` text DEFAULT NULL,"							\
	"`created` datetime NOT NULL DEFAULT current_timestamp(),"			\
	"PRIMARY KEY(`id`),"												\
	"UNIQUE KEY `groupAlias` (`groupAlias`)"

#define GROUP_TABLE_SCHEMA_SQLITE										\
	"`id` INTEGER PRIMARY KEY,"						\
	"`name` TEXT NOT NULL,"										\
	"`groupAlias` TEXT NOT NULL UNIQUE,"								    \
	"`description` TEXT DEFAULT NULL,"							\
	"`created` integer(4) not null default (strftime('%s','now'))"			

#define GROUP_TABLE_LAST_SCHEMA_VERSION 1

namespace model {
	namespace table {

		typedef Poco::Tuple<uint64_t, std::string, std::string, std::string, Poco::DateTime> GroupTuple;

		class Group : public BaseTable
		{
		public:
			Group();
			Group(const std::string& name, const std::string& alias, const std::string& description);
			Group(const GroupTuple& group);

			static std::unique_ptr<Group> load(const std::string& alias);
			static std::unique_ptr<Group> load(uint64_t id);

			// insert or update if id is != 0
			void save(Poco::Data::Session& dbSession);

			const char* tableName() const { return getTableName(); }
			static const char* getTableName() { return "group"; }
			int getLastSchemaVersion() const { return GROUP_TABLE_LAST_SCHEMA_VERSION; }
			const char* getSchema() const { return ConnectionManager::getInstance()->isSqlite() ? GROUP_TABLE_SCHEMA_SQLITE : GROUP_TABLE_SCHEMA; }

			const std::string& getName() const { return mName; }
			const std::string& getDescription() const { return mDescription; }

			inline void setName(const std::string& name) { mName = name; }
			inline void setDescription(const std::string& description) { mDescription = description; }

		protected:
			std::string mName;
			std::string mAlias;
			std::string mDescription;
			Poco::DateTime mCreated;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_GROUP_H