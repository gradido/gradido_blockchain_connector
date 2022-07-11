#include "TransactionClientDetail.h"
#include "../../ConnectionManager.h"
#include "sodium.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {
		TransactionClientDetail::TransactionClientDetail()
			: mClientTransactionId(0), mTransactionNr(0)
		{

		}
		TransactionClientDetail::TransactionClientDetail(uint64_t clientTransactionId, const std::string& iotaMessageId)
			: mClientTransactionId(clientTransactionId), mTransactionNr(0), mIotaMessageId(iotaMessageId),
			mPendingState(PendingTransactions::PendingTransaction::State::SENDED)
		{

		}

		TransactionClientDetail::TransactionClientDetail(const TransactionClientDetailTuple& tuple)
			: BaseTable(tuple.get<0>()), mClientTransactionId(tuple.get<1>()), mTransactionNr(tuple.get<2>()), 
			mIotaMessageId(tuple.get<3>()), mTxHash(tuple.get<4>()), 
			mPendingState((PendingTransactions::PendingTransaction::State)tuple.get<5>()), 
			mError(tuple.get<6>()),
			mCreated(tuple.get<7>()), mUpdated(tuple.get<8>())
		{

		}

		TransactionClientDetail::~TransactionClientDetail()
		{

		}

		std::unique_ptr<TransactionClientDetail> TransactionClientDetail::load(uint64_t id)
		{
			auto session = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(session);
			selectFields(select);
			TransactionClientDetailTuple tuple;
			select << "where id = ?", use(id), into(tuple), now;
			return std::make_unique<TransactionClientDetail>(tuple);

		}
		std::unique_ptr<TransactionClientDetail> TransactionClientDetail::findByClientTransactionNr(uint64_t clientTransactionNr)
		{
			auto session = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(session);
			selectFields(select);
			TransactionClientDetailTuple tuple;
			select << "where client_transaction_id = ?", use(clientTransactionNr), into(tuple), now;
			return std::make_unique<TransactionClientDetail>(tuple);
		}
		std::vector<std::unique_ptr<TransactionClientDetail>> TransactionClientDetail::findByTransactionNr(uint64_t transactionNr)
		{
			auto session = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(session);
			selectFields(select);
			std::vector<TransactionClientDetailTuple> tuples;
			select << "where transaction_nr = ?", use(transactionNr), into(tuples), now;

			std::vector<std::unique_ptr<TransactionClientDetail>> result;
			result.reserve(tuples.size());
			for (TransactionClientDetailTuple tuple : tuples) {
				result.push_back(std::make_unique<TransactionClientDetail>(tuple));
			}
			return result;
			
		}
		std::vector<std::unique_ptr<TransactionClientDetail>> TransactionClientDetail::findByIotaMessageId(const std::string& iotaMessageId)
		{
			auto session = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(session);
			selectFields(select);
			std::vector<TransactionClientDetailTuple> tuples;
			select << "where iota_message_id = ?", useRef(iotaMessageId), into(tuples), now;

			std::vector<std::unique_ptr<TransactionClientDetail>> result;
			result.reserve(tuples.size());
			for (TransactionClientDetailTuple tuple : tuples) {
				result.push_back(std::make_unique<TransactionClientDetail>(tuple));
			}
			return result;
		}

		std::string TransactionClientDetail::calculateMessageId(const model::gradido::GradidoTransaction* transaction)
		{
			unsigned char hash[crypto_generichash_BYTES];
			auto rawMessage = transaction->getSerializedConst();
			crypto_generichash(
				hash, sizeof hash,
				(const unsigned char*)rawMessage->data(),
				rawMessage->size(),
				NULL, 0
			);
			return std::string((char*)hash, sizeof hash);
		}

		void TransactionClientDetail::save()
		{
			save(ConnectionManager::getInstance()->getConnection());
		}

		void TransactionClientDetail::save(Poco::Data::Session& dbSession)
		{
			char pendingStateChar = static_cast<char>(mPendingState);
			if (mID) {
				Poco::Data::Statement update(dbSession);
				update << "UPDATE `" << getTableName()
					<< "` SET transaction_nr=?, iota_message_id=?, tx_hash=?, pending_state=?, error=? "
					<< "WHERE id = ?",
					use(mTransactionNr), use(mIotaMessageId), use(mTxHash), use(pendingStateChar), use(mError), use(mID), now;
			}
			else {
				Poco::Data::Statement insert(dbSession);
				
				insert << "INSERT INTO `" << getTableName()
					<< "` (client_transaction_id, transaction_nr, iota_message_id, tx_hash, pending_state) VALUES(?,?,?,?,?)",
					use(mClientTransactionId), use(mTransactionNr), use(mIotaMessageId), use(mTxHash), use(pendingStateChar), now;
			}
		}


		void TransactionClientDetail::selectFields(Poco::Data::Statement& select)
		{
			select << "SELECT id, client_transaction_id, transaction_nr, iota_message_id, tx_hash, pending_state, error, created, updated from "
				<< getTableName() << " ";
		}
	}
}