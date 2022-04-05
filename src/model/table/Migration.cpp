#include "Migration.h"
#include "ConnectionManager.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {
		Migration::Migration()
			: mVersion(0)
		{
		}

		Migration::Migration(const std::string& tableName, int version)
			: mTableName(tableName), mVersion(version)
		{

		}

		Migration::Migration(const MigrationTuple& tuple)
			: BaseTable(tuple.get<0>()), mTableName(tuple.get<1>()), mVersion(tuple.get<2>())
		{

		}

		std::unique_ptr<Migration> Migration::load(const std::string& tableName)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			auto migration = std::make_unique<Migration>();

			select << "SELECT id, table_name, version "
				<< " from " << getTableName()
				<< " where table_name LIKE ?",
				into(migration->mTableName), into(migration->mVersion), useRef(tableName);

			if (!select.execute()) {
				throw RowNotFoundException("couldn't load migration", getTableName(), "where table_name LIKE " + tableName);
			}
			return std::move(migration);
		}

		std::vector<MigrationTuple> Migration::loadAll()
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			std::vector<MigrationTuple> migrations;
			Poco::Data::Statement select(dbSession);
			select << "SELECT id, table_name, version from " << getTableName(),
				into(migrations), now;

			return std::move(migrations);
		}

		void Migration::save(Poco::Data::Session& dbSession)
		{
			Poco::Data::Statement insert(dbSession);

			insert << "INSERT INTO " << getTableName() << "(table_name, version) VALUES(?,?);",
				use(mTableName), use(mVersion), now;			
		}
	}
}