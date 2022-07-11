#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_TABLE_BASE_TABLE_H

#include "ServerConfig.h"
#include "Poco/Tuple.h"
#include "Poco/Data/Session.h"

namespace model {
	namespace table {
		class BaseTable
		{
		public:
			BaseTable() : mID(0) {}
			BaseTable(uint64_t id) : mID(id) {}
			virtual ~BaseTable() {}
			virtual const char* tableName() const = 0;
			virtual int getLastSchemaVersion() const = 0;
			virtual const char* getSchema() const = 0;

			inline uint64_t getId() const { return mID; }
			inline void setId(uint64_t id) { mID = id; }

			virtual void save(Poco::Data::Session& dbSession) = 0;
			uint64_t getLastInsertId(Poco::Data::Session& dbSession);

			void remove();
			void remove(Poco::Data::Session& dbSession);

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