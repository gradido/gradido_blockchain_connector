#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_GROUP_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_GROUP_H

#include "BaseTable.h"


#define GROUP_TABLE_SCHEMA												\
	"`id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,"						\
	"`name` varchar(150) NOT NULL,"										\
	"`alias` bigint unsigned NOT NULL,"								    \
	"`description` binary(32) DEFAULT NULL,"							\
	"`created` datetime NOT NULL DEFAULT current_timestamp(),"			\
	"PRIMARY KEY(`id`),"												\
	"UNIQUE KEY `alias` (`alias`)"

#define GROUP_TABLE_LAST_SCHEMA_VERSION 1

namespace model {
	namespace table {
		class Group : public BaseTable
		{
		public:

		protected:
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_GROUP_H