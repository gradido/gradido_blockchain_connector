#include "UpdateCreationTargetDate.h"
#include "../ConnectionManager.h"
#include "ServerConfig.h"
#include "../model/import/CommunityServer.h"

using namespace Poco::Data::Keywords;

namespace task
{
	UpdateCreationTargetDate::UpdateCreationTargetDate(uint64_t stateUserId)
		: CPUTask(ServerConfig::g_WorkerThread), mStateUserId(stateUserId)
	{

	}
	UpdateCreationTargetDate::~UpdateCreationTargetDate()
	{

	}

	int UpdateCreationTargetDate::run()
	{
		auto dbSession = ConnectionManager::getInstance()->getConnection();
		Poco::Data::Statement select(dbSession);
		typedef Poco::Tuple<uint64_t, std::string, Poco::DateTime, uint64_t, Poco::DateTime> CreationTransactionTuple;
		std::list<CreationTransactionTuple> creationTransactions;
		select << "select t.id, t.memo, t.received, tc.amount, tc.target_date "
			<< "from " << model::import::CommunityServer::mTempCreationTableName << " as tc "
			<< "JOIN " << model::import::CommunityServer::mTempTransactionsTableName <<" as t "
			<< "ON t.id = tc.transaction_id "
			<< "where tc.state_user_id = ? and t.received = tc.target_date and tc.amount <= 10000000;", use(mStateUserId), into(creationTransactions);

		std::for_each(creationTransactions.begin(), creationTransactions.end(), [&](const CreationTransactionTuple& tuple) {

			});
		return 0;
	}

}