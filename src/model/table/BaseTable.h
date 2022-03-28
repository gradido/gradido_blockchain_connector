#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H

#include "ServerConfig.h"

namespace model {
	namespace table {
		class BaseTable
		{
		public:
			virtual ~BaseTable() {}
			virtual const char* tableName() = 0;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H