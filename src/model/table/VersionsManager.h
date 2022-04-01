#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H

#include <string>

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
			void createTableIfNotExist(const std::string& tablename, const std::string& tableDefinition);

			VersionsManager() {};
		};

	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H