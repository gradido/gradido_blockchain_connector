#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H

#include "BaseTable.h"
#include "gradido_blockchain/MemoryManager.h"

#ifndef LI_SYMBOL_name
#define LI_SYMBOL_name
LI_SYMBOL(name)
#endif

#ifndef LI_SYMBOL_public_key
#define LI_SYMBOL_public_key
LI_SYMBOL(public_key)
#endif

#ifndef LI_SYMBOL_encrypted_private_key
#define LI_SYMBOL_encrypted_private_key
LI_SYMBOL(encrypted_private_key)
#endif
namespace li {
	template <unsigned SIZE> struct sql_binary : public std::string {
		using std::string::string;
		using std::string::operator=;

		sql_binary() : std::string() {}
	};

	template <unsigned S> inline std::string cpptype_to_mysql_type(const sql_binary<S>) {
		std::ostringstream ss;
		ss << "BINARY(" << S << ')';
		return ss.str();
	}

	template <unsigned S> inline auto type_to_mysql_statement_buffer_type(const sql_binary<S>) {
		return MYSQL_TYPE_STRING;
	}
}


#define USER_TABLE_SCHEMA_1(DB)											\
		li::sql_orm_schema(DB, "user")									\
		.fields(													    \
				s::id(s::auto_increment, s::primary_key) = int(),		\
				s::name = li::sql_varchar<255>(),						    \
				s::password = long long(),								\
				s::public_key = li::sql_binary<32>(),					    \
				s::encrypted_private_key = li::sql_binary<80>()				\
		)


#define USER_TABLE_LAST_SCHEMA_VERSION 1
#define USER_TABLE_LAST_SCHEMA USER_TABLE_SCHEMA_1

namespace model {
	namespace table {
		inline auto getUserConnection()
		{
			auto userSchema = USER_TABLE_LAST_SCHEMA(*ServerConfig::g_Mysql);
			return userSchema.connect();
		}
	}
}



#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_USER_H