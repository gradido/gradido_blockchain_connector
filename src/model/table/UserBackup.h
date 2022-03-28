#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_BACKUP_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_BACKUP_H

#include "BaseTable.h"


#ifndef LI_SYMBOL_user_id
#define LI_SYMBOL_user_id
LI_SYMBOL(user_id)
#endif

#ifndef LI_SYMBOL_key_user_id
#define LI_SYMBOL_key_user_id
LI_SYMBOL(key_user_id)
#endif

#ifndef LI_SYMBOL_passphrase
#define LI_SYMBOL_passphrase
LI_SYMBOL(passphrase)
#endif

#define USER_BACKUP_TABLE_SCHEMA_1(DB)									\
		li::sql_orm_schema(DB, "user_backup")							\
			.fields(s::id(s::auto_increment, s::primary_key) = int(),   \
				s::user_id = int(),										\
				s::key_user_id = int(),									\
				s::passphrase = std::string()							\
)


#define USER_BACKUP_TABLE_LAST_SCHEMA_VERSION 1
#define USER_BACKUP_TABLE_LAST_SCHEMA USER_BACKUP_TABLE_SCHEMA_1

namespace model {
	namespace table {
		inline auto getUserBackupConnection()
		{
			auto userBackupSchema = USER_BACKUP_TABLE_LAST_SCHEMA(*ServerConfig::g_Mysql);
			return userBackupSchema.connect();
		}
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_BACKUP_H
