#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H

#include "ServerConfig.h"

// add missing types for lithium 
// add binary 
/*
namespace li {
	template <unsigned SIZE> struct sql_binary : public sql_varchar<SIZE> {
		using sql_varchar::sql_varchar;
		using sql_varchar::operator=;

		sql_binary() : sql_varchar() {}
	};
	template <unsigned S> inline auto type_to_mysql_statement_buffer_type(const sql_binary<S>) {
		return MYSQL_TYPE_BLOB;
	}

	template <unsigned S> inline std::string cpptype_to_mysql_type(const sql_binary<S>) {
		std::ostringstream ss;
		ss << "BINARY(" << S << ')';
		return ss.str();
	}
	template <unsigned SIZE> void mysql_bind_param(MYSQL_BIND& b, const sql_binary<SIZE>& s) {
		mysql_bind_param_binary(b, *const_cast<std::string*>(static_cast<const std::string*>(&s)));
	}
	inline void mysql_bind_param_binary(MYSQL_BIND& b, std::string& s) {
		b.buffer = &s[0];
		b.buffer_type = MYSQL_TYPE_BLOB;
		b.buffer_length = s.size();
	}
	template <unsigned SIZE>
	void mysql_bind_output(MYSQL_BIND& b, unsigned long* real_length, sql_binary<SIZE>& s) {
		// string are written in static buffer mysql_bind_data::preallocated_string.
		b.buffer_type = MYSQL_TYPE_BLOB;
		b.length = real_length;
	}
}
*/

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

		// ************** Exceptions *************************
		class RowNotFoundException : public GradidoBlockchainException
		{
		public: 
			explicit RowNotFoundException(const char* what, const char* tableName, std::string condition) noexcept;

			std::string getFullString() const;
		protected:
			std::string mTableName;
			std::string mCondition;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H