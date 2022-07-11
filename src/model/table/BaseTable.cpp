#include "BaseTable.h"
#include "../../ConnectionManager.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {
		
		uint64_t BaseTable::getLastInsertId(Poco::Data::Session& dbSession)
		{
			Poco::Data::Statement select(dbSession);
			select << "SELECT LAST_INSERT_ID();", into(mID), now;

			return mID;
		}

		void BaseTable::remove()
		{
			remove(ConnectionManager::getInstance()->getConnection());
		}
		void BaseTable::remove(Poco::Data::Session& dbSession)
		{
			if (mID) {
				Poco::Data::Statement remove(dbSession);
				remove << "DELETE FROM " << tableName() << " where id = ?", use(mID), now;
			}
		}

		RowNotFoundException::RowNotFoundException(const char* what, const char* tableName, std::string condition) noexcept
			: GradidoBlockchainException(what), mTableName(tableName), mCondition(std::move(condition))
		{

		}

		std::string RowNotFoundException::getFullString() const
		{
			std::string result = what();
			result += ", table: " + mTableName;
			result += ", condition: " + mCondition;
			return result;
		}

		
	}
}