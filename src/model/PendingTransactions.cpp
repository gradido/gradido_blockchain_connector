#include "PendingTransactions.h"
#include "Poco/DateTimeFormatter.h"

using namespace rapidjson;

namespace model {

	PendingTransactions::PendingTransaction::PendingTransaction(
		const std::string& _iotaMessageId, 
		model::gradido::TransactionType _transactionType,
		const std::string _apolloCreatedDecay
	)
		: iotaMessageId(_iotaMessageId), transactionType(_transactionType), 
		  state(State::SENDED), 
		  apolloCreatedDecay(_apolloCreatedDecay)
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

	void PendingTransactions::pushNewTransaction(
		const std::string& iotaMessageId,
			model::gradido::TransactionType transactionType,
			const std::string apolloCreatedDecay
		)
	{
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
		mPendingTransactions.push_back(std::move(PendingTransaction(
			iotaMessageId, 
			transactionType,
			apolloCreatedDecay
		)));
		while (mPendingTransactions.size() > MAX_PENDING_TRANSACTIONS_IN_LIST) {
			mPendingTransactions.pop_front();
		}

	}
	void PendingTransactions::updateTransaction(const std::string& iotaMessageId, bool confirmend, const std::string& errorMessage /* = ""*/)
	{
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
		for (auto it = mPendingTransactions.rbegin(); it != mPendingTransactions.rend(); it++) {
			if (memcmp(it->iotaMessageId.data(), iotaMessageId.data(), 64) == 0) {
				if (it->state == PendingTransaction::State::CONFIRMED) return;
				it->state = confirmend ? PendingTransaction::State::CONFIRMED : PendingTransaction::State::REJECTED;
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
		
	bool PendingTransactions::validateApolloCreationDecay(const model::gradido::GradidoTransaction* gradidoTransaction)
	{
		// getaddressbalance
		return true;
	}
	
	
}