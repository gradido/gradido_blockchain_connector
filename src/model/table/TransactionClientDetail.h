#ifndef __GRADIDO_BLOCKCHAIN_MODEL_TABLE_TRANSACTION_CLIENT_DETAIL_H
#define __GRADIDO_BLOCKCHAIN_MODEL_TABLE_TRANSACTION_CLIENT_DETAIL_H

#include "BaseTable.h"
#include "../PendingTransactions.h"

#define TRANSACTION_CLIENT_DETAIL_SCHEMA								\
	"`id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,"						\
	"`client_transaction_id` UNSIGNED NOT NULL,"						\
	"`transaction_nr` UNSIGNED NULL,"								    \
	"`iota_message_id` binary(32) NOT NULL,"							\
    "`tx_hash` binary(32) NULL,"										\
    "`pending_state` tinyint NOT NULL DEFAULT 0,"						\
	"`created` datetime NOT NULL DEFAULT current_timestamp(),"			\
    "`updated` datetime NULL ON UPDATE CURRENT_TIMESTAMP,"				\
	"PRIMARY KEY(`id`),"												\
	"UNIQUE KEY `client_transaction_id` (`client_transaction_id`)"

#define TRANSACTION_CLIENT_DETAIL_TABLE_LAST_SCHEMA_VERSION 1



namespace model {
	namespace table {

		typedef Poco::Tuple<uint64_t, uint64_t, uint64_t, std::string, std::string, char, Poco::DateTime, Poco::DateTime> TransactionClientDetailTuple;

		class TransactionClientDetail : public BaseTable
		{
		public:
			TransactionClientDetail();
			TransactionClientDetail(uint64_t clientTransactionId, const std::string& iotaMessageId);
			TransactionClientDetail(const TransactionClientDetailTuple& tuple);
			~TransactionClientDetail();

			static std::unique_ptr<TransactionClientDetail> load(uint64_t id);
			static std::unique_ptr<TransactionClientDetail> findByClientTransactionNr(uint64_t clientTransactionNr);
			static std::vector<std::unique_ptr<TransactionClientDetail>> findByTransactionNr(uint64_t transactionNr);
			static std::vector<std::unique_ptr<TransactionClientDetail>> findByIotaMessageId(const std::string& iotaMessageId);

			const char* tableName() const { return getTableName(); }
			static const char* getTableName() { return "transaction_client_details"; }
			int getLastSchemaVersion() const { return TRANSACTION_CLIENT_DETAIL_TABLE_LAST_SCHEMA_VERSION; }
			const char* getSchema() const { return TRANSACTION_CLIENT_DETAIL_SCHEMA; }

			void save(Poco::Data::Session& dbSession);

			inline uint64_t getClientTransactionId() const { return mClientTransactionId; }
			inline uint64_t getTransactionNr() const { return mTransactionNr; }
			inline const std::string& getIotaMessageId() const { return mIotaMessageId; }
			inline const std::string& getTxHash() const { return mTxHash; }
			inline auto getPendingState() const { return mPendingState; }
			inline Poco::DateTime getCreated() const { return mCreated; }
			inline Poco::DateTime getUpdated() const { return mUpdated; }

			inline void setTransactionNr(uint64_t transactionNr) { mTransactionNr = transactionNr; }
			inline void setIotaMessageId(const std::string& iotaMessageId) { mIotaMessageId = iotaMessageId; }
			inline void setTxHash(const std::string& txHash) { mTxHash = txHash; }
			inline void setPendingState(PendingTransactions::PendingTransaction::State pendingState) { mPendingState = pendingState; }

		protected:
			uint64_t mClientTransactionId;
			uint64_t mTransactionNr;
			std::string mIotaMessageId;
			std::string mTxHash;
			PendingTransactions::PendingTransaction::State mPendingState;
			Poco::DateTime mCreated;
			Poco::DateTime mUpdated;

			static void selectFields(Poco::Data::Statement& select);
		};
		
	}
}

#endif // __GRADIDO_BLOCKCHAIN_MODEL_TABLE_TRANSACTION_CLIENT_DETAIL_H