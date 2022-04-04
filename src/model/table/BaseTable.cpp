#include "BaseTable.h"

namespace model {
	namespace table {

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