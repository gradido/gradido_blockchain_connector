#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_MIGRATE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_MIGRATE_H

#include "BaseTable.h"

#ifndef LI_SYMBOL_table_name
#define LI_SYMBOL_table_name
LI_SYMBOL(table_name)
#endif

#ifndef LI_SYMBOL_version
#define LI_SYMBOL_version
LI_SYMBOL(version)
#endif

#define MIGRATE_TABLE_SCHEMA(DB, TABLE_NAME)															\
		li::sql_orm_schema(DB, TABLE_NAME)																\
			.fields(s::id(s::auto_increment, s::primary_key) = int(),									\
				s::table_name = std::string(),	s::version = int());									 			



#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_MIGRATE_H
