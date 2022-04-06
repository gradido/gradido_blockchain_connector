#include "JsonGetPendingTransactions.h"
#include "model/PendingTransactions.h"

using namespace rapidjson;

Document JsonGetPendingTransactions::handle(const Document& params)
{
	auto result = stateSuccess();
	auto alloc = result.GetAllocator();
	result.AddMember("pendingTransactions", model::PendingTransactions::getInstance()->listAsJson(alloc), alloc);
	return result;
}