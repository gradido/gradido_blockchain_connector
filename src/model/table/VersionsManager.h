#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H

#include <string>
#include "BaseTable.h"
#include "Poco/Data/Session.h"

namespace model {
	namespace table {
		class VersionsManager
		{
		public:
			~VersionsManager() {};
			static VersionsManager* getInstance();

			// check if all tables are in the current version, if not update
			void migrate();
			
		protected:
			void createTableIfNotExist(Poco::Data::Session& dbSession, const std::string& tablename, const std::string& tableDefinition);

			std::unique_ptr<BaseTable> factoryTable(const std::string& tableName);

			VersionsManager();
		};

		// *******************  Exceptions ****************************************************
		class UnknownTableNameException : public GradidoBlockchainException
		{
		public:
			explicit UnknownTableNameException(const char* what, const std::string& tableName) noexcept;
			std::string getFullString() const;

		protected:
			std::string mTableName;
		};

		class MissingMigrationException : public GradidoBlockchainException
		{
		public: 
			explicit MissingMigrationException(const char* what, const std::string& tableName, uint32_t version) noexcept;
			std::string getFullString() const;

		protected:
			std::string mTableName;
			uint32_t mVersion;
		};

	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H