#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H

#include "lithium_symbol.hh"

#ifndef LI_SYMBOL_my_symbol
#define LI_SYMBOL_my_symbol
LI_SYMBOL(my_symbol)
#endif

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
			VersionsManager() {};
		};

	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_VERSIONS_MANAGER_H