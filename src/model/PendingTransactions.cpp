#include "PendingTransactions.h"
#include "Poco/DateTimeFormatter.h"
#include "table/TransactionClientDetail.h"
#include "../ConnectionManager.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

using namespace rapidjson;

namespace model {

	PendingTransactions::PendingTransaction::PendingTransaction(
		const std::string& _iotaMessageId, 
		model::gradido::TransactionType _transactionType,
		const std::string& _apolloCreatedDecay,
		Poco::DateTime _apolloDecayStart,
		uint64_t	   _apolloTransactionId
	)
		: iotaMessageId(_iotaMessageId), transactionType(_transactionType), 
		  state(State::SENDED), 
		  apolloCreatedDecay(_apolloCreatedDecay), apolloDecayStart(_apolloDecayStart), apolloTransactionId(_apolloTransactionId)
	{

	}

	const char* PendingTransactions::PendingTransaction::StateToString(State type)
	{
		switch (type)
		{
		case State::SENDED:
			return "sended";
			break;
		case State::CONFIRMED:
			return "confirmed";
			break;
		case State::REJECTED:
			return "rejected";
			break;
		default:
			break;
		}
		return "unknown";
	}
	

	PendingTransactions* PendingTransactions::getInstance()
	{
		static PendingTransactions one;
		return &one;
	}

	void PendingTransactions::pushNewTransaction(PendingTransaction pendingTransaction) 
	{
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
		mPendingTransactions.push_back(std::move(pendingTransaction));
		while (mPendingTransactions.size() > MAX_PENDING_TRANSACTIONS_IN_LIST) {
			mPendingTransactions.pop_front();
		}
		model::table::TransactionClientDetail transactionClientDetail(pendingTransaction.apolloTransactionId, pendingTransaction.iotaMessageId);
		transactionClientDetail.save(ConnectionManager::getInstance()->getConnection());
	}
	void PendingTransactions::updateTransaction(const std::string& iotaMessageId, bool confirmend, const std::string& errorMessage /* = ""*/)
	{
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
		auto newPendingState = confirmend ? PendingTransaction::State::CONFIRMED : PendingTransaction::State::REJECTED;

		auto iotaMessageBin = DataTypeConverter::hexToBinString(iotaMessageId)->substr(0, 32);
		auto transactionClientDetail = model::table::TransactionClientDetail::findByIotaMessageId(iotaMessageBin);
		assert(transactionClientDetail.size() <= 1);
		if (transactionClientDetail.size() == 1) {
			transactionClientDetail[0]->setPendingState(newPendingState);
			transactionClientDetail[0]->save(ConnectionManager::getInstance()->getConnection());
		}

		for (auto it = mPendingTransactions.rbegin(); it != mPendingTransactions.rend(); it++) {
			if (memcmp(it->iotaMessageId.data(), iotaMessageId.data(), 64) == 0) {
				if (it->state == PendingTransaction::State::CONFIRMED) return;
				it->state = newPendingState;
				it->errorMessage = errorMessage;
				return;
			}
		}
		
	}

	Value PendingTransactions::listAsJson(Document::AllocatorType& alloc) const
	{
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
		Value list(kArrayType);
		std::for_each(mPendingTransactions.rbegin(), mPendingTransactions.rend(), [&](const PendingTransaction& pending) {
			Value entry(kObjectType);
			entry.AddMember("messageId", Value(pending.iotaMessageId.data(), alloc), alloc);
			entry.AddMember("transactionType", Value(model::gradido::TransactionBase::getTransactionTypeString(pending.transactionType), alloc), alloc);
			// timestamp in ms
			entry.AddMember("created", pending.created.timestamp().epochMicroseconds()/1000, alloc);
			entry.AddMember("state", Value(pending.getStateString(), alloc), alloc);
			entry.AddMember("errorMessage", Value(pending.errorMessage.data(), alloc), alloc);
			list.PushBack(entry, alloc);
		});
		return list;
	}

	std::list<PendingTransactions::PendingTransaction>::const_iterator PendingTransactions::findTransaction(const std::string& iotaMessageId) const
	{
		for (auto it = mPendingTransactions.begin(); it != mPendingTransactions.end(); it++) {
			if (it->iotaMessageId == iotaMessageId) {
				return it;
			}
		}
		return mPendingTransactions.end();
	}

	bool PendingTransactions::isConfirmed(const std::string& iotaMessageId) const
	{
		auto it = findTransaction(iotaMessageId);
		if (it == mPendingTransactions.end()) {
			return false;
		}
		return it->state == PendingTransaction::State::CONFIRMED;

	}
	bool PendingTransactions::isRejected(const std::string& iotaMessageId) const
	{
		auto it = findTransaction(iotaMessageId);
		if (it == mPendingTransactions.end()) {
			return false;
		}
		return it->state == PendingTransaction::State::REJECTED;
	}
		
	bool PendingTransactions::validateApolloCreationDecay(const model::gradido::GradidoTransaction* gradidoTransaction)
	{
		// getaddressbalance
		return true;
	}
	
	
}