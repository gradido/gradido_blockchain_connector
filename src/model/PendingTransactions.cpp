#include "PendingTransactions.h"
#include "Poco/DateTimeFormatter.h"

using namespace rapidjson;

namespace model {

	PendingTransaction::PendingTransaction(const std::string& _iotaMessageId, model::gradido::TransactionType _transactionType)
		: iotaMessageId(_iotaMessageId), transactionType(_transactionType), pendingType(PENDING_SENDED)
	{

	}

	PendingTransactions* PendingTransactions::getInstance()
	{
		static PendingTransactions one;
		return &one;
	}

	void PendingTransactions::pushNewTransaction(const std::string& iotaMessageId, model::gradido::TransactionType transactionType)
	{
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
		mPendingTransactions.push_back(std::move(PendingTransaction(iotaMessageId, transactionType)));
		while (mPendingTransactions.size() > MAX_PENDING_TRANSACTIONS_IN_LIST) {
			mPendingTransactions.pop_front();
		}

	}
	void PendingTransactions::updateTransaction(const std::string& iotaMessageId, bool confirmend, const std::string& errorMessage /* = ""*/)
	{
		std::scoped_lock<std::recursive_mutex> _lock(mWorkMutex);
		for (auto it = mPendingTransactions.rbegin(); it != mPendingTransactions.rend(); it++) {
			if (it->iotaMessageId == iotaMessageId) {
				if (it->pendingType == PENDING_CONFIRMED) return;
				it->pendingType = confirmend ? PENDING_CONFIRMED : PENDING_REJECTED;
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
			entry.AddMember("pendingType", Value(pendingTypeToString(pending.pendingType), alloc), alloc);
			entry.AddMember("errorMessage", Value(pending.errorMessage.data(), alloc), alloc);
			list.PushBack(entry, alloc);
		});
		return list;
	}

	const char* PendingTransactions::pendingTypeToString(PendingType type)
	{
		switch (type)
		{
		case model::PENDING_SENDED:
			return "sended";
			break;
		case model::PENDING_CONFIRMED:
			return "confirmed";
			break;
		case model::PENDING_REJECTED:
			return "rejected";
			break;
		default:
			break;
		}
		return "unknown";
	}
}