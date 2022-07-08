#include "Group.h"
#include "ConnectionManager.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {

		Group::Group()
		{

		}
		Group::Group(const std::string& name, const std::string& alias, const std::string& description)
			: mName(name), mAlias(alias), mDescription(description)
		{

		}

		Group::Group(const GroupTuple& group)
			: BaseTable(group.get<0>()), mName(group.get<1>()), mAlias(group.get<2>()), 
			mDescription(group.get<3>()), mCreated(group.get<4>())
		{

		}

		std::unique_ptr<Group> Group::load(const std::string& alias)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);

			GroupTuple groupTuple;

			select << "SELECT id, name, groupAlias, description, created "
				<< " from `" << getTableName() << "`"
				<< " where groupAlias LIKE ?",
				into(groupTuple), useRef(alias);

			if (!select.execute()) {
				throw RowNotFoundException("couldn't load group", getTableName(), "where groupAlias LIKE " + alias);
			}
			auto group = std::make_unique<Group>(groupTuple);
			return std::move(group);
		}

		std::unique_ptr<Group> Group::load(uint64_t id)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);

			GroupTuple groupTuple;

			select << "SELECT id, name, groupAlias, description, created "
				<< " from `" << getTableName() << "`"
				<< " where id = ?",
				into(groupTuple), useRef(id);

			if (!select.execute()) {
				throw RowNotFoundException("couldn't load group", getTableName(), "where id = " + id);
			}
			auto group = std::make_unique<Group>(groupTuple);
			return std::move(group);
		}

		void Group::save(Poco::Data::Session& dbSession)
		{

			if (mID) {
				Poco::Data::Statement update(dbSession);
				update << "UPDATE `" << getTableName()
					<< "` SET name=?, description=? "
					<< "WHERE id = ?",
					use(mName), use(mDescription), use(mID), now;
			}
			else {
				Poco::Data::Statement insert(dbSession);

				insert << "INSERT INTO `" << getTableName() << "` (name, groupAlias, description) VALUES(?,?,?);",
					use(mName), use(mAlias), use(mDescription), now;
			}
		}
	}
}