#include "Group.h"
#include "ConnectionManager.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {

		Group::Group()
		{

		}
		Group::Group(const std::string& name, const std::string& alias, const std::string& description, uint32_t coinColor)
			: mName(name), mAlias(alias), mDescription(description), mCoinColor(coinColor)
		{

		}

		Group::Group(const GroupTuple& group)
			: BaseTable(group.get<0>()), mName(group.get<1>()), mAlias(group.get<2>()), 
			mDescription(group.get<3>()), mCoinColor(group.get<4>()), mCreated(group.get<5>())
		{

		}

		std::unique_ptr<Group> Group::load(const std::string& alias)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);

			GroupTuple groupTuple;

			select << "SELECT id, name, groupAlias, description, coinColor, created "
				<< " from `" << getTableName() << "`"
				<< " where groupAlias LIKE ?",
				into(groupTuple), useRef(alias);

			if (!select.execute()) {
				throw RowNotFoundException("couldn't load group", getTableName(), "where groupAlias LIKE " + alias);
			}
			auto group = std::make_unique<Group>(groupTuple);
			return std::move(group);
		}

		void Group::save(Poco::Data::Session& dbSession)
		{
			Poco::Data::Statement insert(dbSession);

			insert << "INSERT INTO `" << getTableName() << "` (name, groupAlias, description, coinColor) VALUES(?,?,?,?);",
				use(mName), use(mAlias), use(mDescription), use(mCoinColor), into(mID), now;
		}
	}
}