#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H

#include "ServerConfig.h"

namespace model {
	namespace table {
		class BaseTable
		{
		public:
			BaseTable() : mID(0) {}
			BaseTable(uint64_t id) : mID(id) {}
			virtual ~BaseTable() {}
			virtual const char* tableName() const = 0;

			inline uint64_t getId() const { return mID; }
			inline void setId(uint64_t id) { mID = id; }

			virtual uint64_t save() = 0;
		protected:
			uint64_t mID;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H