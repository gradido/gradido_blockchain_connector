#include "GradidoBlockchainConnector.h"
#include "ServerConfig.h"
#include "JSONInterface/JsonRequestHandlerFactory.h"
#include "model/table/VersionsManager.h"
#include "GradidoNodeRPC.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "gradido_blockchain/http/ServerConfig.h"

#include "Poco/Util/HelpFormatter.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Environment.h"
#include "Poco/Logger.h"
#include "Poco/Path.h"
#include "Poco/AsyncChannel.h"
#include "Poco/SimpleFileChannel.h"
#include "Poco/FileChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/SplitterChannel.h"

#include <sodium.h>
#include <google/protobuf/stubs/common.h>

// TODO: move
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionsManagerBlockchain.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"
#include "gradido_blockchain/lib/MultithreadQueue.h"
#include "gradido_blockchain/lib/Decimal.h"
#include "JSONInterface/JsonTransaction.h"
#include "model/import/LoginServer.h"
#include "model/import/CommunityServer.h"
#include "model/import/ApolloServer.h"
#include "model/table/TransactionClientDetail.h"
#include "model/PendingTransactions.h"
#include "gradido_blockchain/lib/Decay.h"
#include "task/SendTransactionToGradidoNode.h"
#include "task/CompareApolloWithGradidoNode.h"
#include "task/CPUTaskGroup.h"
#include "plugins/apollo/DecayDecimal.h"

#include "Poco/FileStream.h"

#include "ConnectionManager.h"
using namespace Poco::Data::Keywords;

GradidoBlockchainConnector::GradidoBlockchainConnector()
	: _helpRequested(false), mArchivedTransactionsHandling(ArchivedTransactionsHandling::NONE)
{
}

GradidoBlockchainConnector::~GradidoBlockchainConnector()
{
}


void GradidoBlockchainConnector::initialize(Application& self)
{
	loadConfiguration(); // load default configuration files, if present
	ServerApplication::initialize(self);
}

void GradidoBlockchainConnector::uninitialize()
{
	ServerApplication::uninitialize();
}

void GradidoBlockchainConnector::defineOptions(Poco::Util::OptionSet& options)
{
	ServerApplication::defineOptions(options);
	
	options.addOption(
		Poco::Util::Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
	);
	options.addOption(
		Poco::Util::Option("config", "c", "use non default config file (default is ~/.gradido/blockchain_connector.properties)", false)
		.repeatable(false)
		.argument("Gradido_LoginServer.properties", true)
	);
	// on windows use / instead of - or -- for options!
	options.addOption(
		Poco::Util::Option("checkBackup", "cb", "check db backup contain valid data, argument is backup type: community for backup in login- and community server format, apollo for apollo server format", false)
		.repeatable(false) // TODO: make repeatable if more than one type is supported
		.argument("community", true)
	);

	options.addOption(
		Poco::Util::Option("sendBackup", "sb", "send db backup transactions to gradidoNode or iota")
		.repeatable(false)
		.argument("gradidoNode", true)
	);
	options.addOption(
		Poco::Util::Option("compareWithGradidoNode", "cp", "compare db data with gradido node, type: iota or community")
		.repeatable(false)
		.argument("apollo", true)
	);
}

void GradidoBlockchainConnector::handleOption(const std::string& name, const std::string& value)
{
	
	if (name == "config") {
		mConfigPath = value;
		return;
	}
	else if (name == "checkBackup") {
		if (value == "community") {
			mArchivedTransactionsHandling = (ArchivedTransactionsHandling)(
				(int)mArchivedTransactionsHandling | (int)ArchivedTransactionsHandling::USE_COMMUNITY_SERVER_DATA
			);
		}
		else if (value == "apollo") {
			mArchivedTransactionsHandling = (ArchivedTransactionsHandling)(
				(int)mArchivedTransactionsHandling | (int)ArchivedTransactionsHandling::USE_APOLLO_SERVER_DATA
			);
		}
		return;
	}
	else if (name == "sendBackup") {
		if (value == "gradidoNode") {
			mArchivedTransactionsHandling = (ArchivedTransactionsHandling)(
				(int)mArchivedTransactionsHandling | (int)ArchivedTransactionsHandling::SEND_DATA_TO_GRADIDO_NODE
			);
		} 
		else if (value == "iota") {
			mArchivedTransactionsHandling = (ArchivedTransactionsHandling)(
				(int)mArchivedTransactionsHandling | (int)ArchivedTransactionsHandling::SEND_DATA_TO_IOTA
			);
		}
		return;
	}
	else if (name == "compareWithGradidoNode") {
		if (value == "apollo") {
			mArchivedTransactionsHandling = (ArchivedTransactionsHandling)(
				(int)mArchivedTransactionsHandling | (int)ArchivedTransactionsHandling::USE_APOLLO_SERVER_DATA
			);
		}
		else if (value == "community") {
			mArchivedTransactionsHandling = (ArchivedTransactionsHandling)(
				(int)mArchivedTransactionsHandling | (int)ArchivedTransactionsHandling::USE_COMMUNITY_SERVER_DATA
			);
		}
		mArchivedTransactionsHandling = (ArchivedTransactionsHandling)(
			(int)mArchivedTransactionsHandling | (int)ArchivedTransactionsHandling::COMPARE_DATA_WITH_GRADIDO_NODE
		);
	}
	ServerApplication::handleOption(name, value);
	if (name == "help") _helpRequested = true;
}

void GradidoBlockchainConnector::displayHelp()
{
	Poco::Util::HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("OPTIONS");
	helpFormatter.setHeader("Gradido Blockchain Connector");
	helpFormatter.format(std::cout);
}

void GradidoBlockchainConnector::checkCommunityServerStateBalances(const std::string& groupAlias)
{
	Poco::Logger& speedLog = Poco::Logger::get("speedLog");
	Poco::Logger& errorLog = Poco::Logger::get("errorLog");
	// code for loading old transactions from db
	/*model::import::LoginServer loginServerImport;
	try {
		loginServerImport.loadAll();
	}
	catch (GradidoBlockchainException& ex) {
		printf("error by importing from login server: %s\n", ex.getFullString().data());
	}*/
	model::import::CommunityServer communityServerImport;
	try {
		communityServerImport.loadAll(groupAlias, true);
	}
	catch (GradidoBlockchainException& ex) {
		printf("error by importing from community server: %s\n", ex.getFullString().data());
	}
	auto tm = model::TransactionsManager::getInstance();
	auto mm = MemoryManager::getInstance();
	Profiler caluclateBalancesTime;
	Poco::DateTime now;
	//auto userBalances = tm->calculateFinalUserBalances("gdd1", now);
	int i = 0;
	auto stateBalances = communityServerImport.getStateBalances();
	auto stateUserTransactions = communityServerImport.getStateUserTransactions();
	auto dbBalance = mm->getMathMemory();
	auto calcBalance = mm->getMathMemory();
	auto diff = MathMemory::create();
	initDefaultDecayFactors();
	for (auto it = stateUserTransactions.begin(); it != stateUserTransactions.end(); it++) {
		//if (it->first < 82) continue;
		if (i > 0) break;
		auto pubkeyHex = communityServerImport.getUserPubkeyHex(it->first);
		/*for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
			auto userBalance = tm->calculateUserBalanceUntil(groupAlias, pubkeyHex, it2->second.balanceDate);
			mpfr_set_str(calcBalance, userBalance.balance.data(), 10, gDefaultRound);
			mpfr_set_ui(dbBalance, it2->second.balanceGddCent, gDefaultRound);
			mpfr_div_d(dbBalance, dbBalance, 10000, gDefaultRound);

			mpfr_sub(diff->getData(), calcBalance, dbBalance, gDefaultRound);
			std::string balanceString = std::to_string((double)it2->second.balanceGddCent / 10000.0);
			//printf("%d balance from db: %s, calculated balance: %s, diff: ", j++, balanceString.data(), userBalance.balance.data());
			mpfr_printf("%.20Rf\n", diff->getData());
		}*/
		auto lastStateUserTransaction = it->second.rbegin()->second;
		try {
			auto userBalance = tm->calculateUserBalanceUntil(groupAlias, pubkeyHex, lastStateUserTransaction.balanceDate);
			mpfr_set_str(calcBalance, userBalance.balance.data(), 10, gDefaultRound);
			mpfr_set_ui(dbBalance, lastStateUserTransaction.balanceGddCent, gDefaultRound);
			mpfr_div_d(dbBalance, dbBalance, 10000, gDefaultRound);
			mpfr_sub(diff->getData(), calcBalance, dbBalance, gDefaultRound);

			std::string balanceString = std::to_string((double)lastStateUserTransaction.balanceGddCent / 10000.0);
			if (mpfr_cmp(dbBalance, calcBalance)) {
			//if (mpfr_cmp_ui(diff->getData(), 5) > 0) {
				printf("balance from db: %s, calculated balance: %s\n", balanceString.data(), userBalance.balance.data());
				printf("state user id: %d, pubkey hex: %s\n", it->first, pubkeyHex.data());
				auto transactionsString = tm->getUserTransactionsDebugString(groupAlias, pubkeyHex);
				printf("%s\n", transactionsString.data());
				i++;
			}
		}
		catch (model::TransactionsManager::AccountInGroupNotFoundException& ex) {
			errorLog.error("error by calculating user balances, account for state user: %u not found", (unsigned)it->first);
		}
	}
	mm->releaseMathMemory(dbBalance);
	mm->releaseMathMemory(calcBalance);

	speedLog.information("calculate user balances time: %s", caluclateBalancesTime.string());
}

void GradidoBlockchainConnector::checkApolloServerDecay(const std::string& groupAlias)
{
	Poco::Logger& speedLog = Poco::Logger::get("speedLog");
	Poco::Logger& errorLog = Poco::Logger::get("errorLog");
	Profiler timeUsed;
	// code for loading old transactions from db
	model::import::ApolloServer apolloServerImport(groupAlias);
	try {
		apolloServerImport.loadAll(groupAlias);
	}
	catch (GradidoBlockchainException& ex) {
		printf("error by importing from apollo server: %s\n", ex.getFullString().data());
	}
	auto tm = model::TransactionsManager::getInstance();
	auto mm = MemoryManager::getInstance();
	Profiler caluclateBalancesTime;
	Poco::DateTime now;
	//auto userBalances = tm->calculateFinalUserBalances("gdd1", now);
	int i = 0;
	Decimal dbBalance;
	Decimal dbDecay;
	Decimal calcBalance;
	Poco::FileOutputStream csvFile("diff.csv");
	csvFile << "id, balance, decay, seconds, diff, cutDiff, cutDiff2, fDiff1, fDiff2, aDiff1" << std::endl;
	auto amount = mm->getMathMemory();
	auto decayForDuration = mm->getMathMemory();
	Decimal calcDecay;
	Decimal diff;
	initDefaultDecayFactors();

	Decimal apolloDecayFactor1("0.99999997803504048973201202316767079413460520837376");
	Decimal apolloDecayFactor2("0.9999999780350404897320120");
	
	//mpfr_set_str(gDecayFactorGregorianCalender, apolloDecayValueString.data(), 10, MPFR_RNDZ);

	auto users = apolloServerImport.getUserIdPubkeyHexMap();
	auto dbSession = ConnectionManager::getInstance()->getConnection();
	Poco::Data::Statement select(dbSession);
	typedef Poco::Tuple<uint64_t, uint64_t, std::string, Poco::DateTime, std::string, Poco::Nullable<Poco::DateTime>, std::string> TransactionTuple;
	std::vector<TransactionTuple> apolloTransactions;
	uint64_t userId = 0;
	select << "select id, previous, CONVERT(balance, varchar(45)), balance_date, CONVERT(decay, varchar(45)), decay_start, CONVERT(amount, varchar(45)) "
		   << "from transactions_temp "
		   << "where user_id = ? "
		   //<< "AND type_id IN (1,2)"
		   << "order by balance_date ASC"
		   , use(userId), into(apolloTransactions);

	for (auto itUser = users->begin(); itUser != users->end(); itUser++) {
		userId = itUser->first;
		auto userPublicKeyHex = itUser->second;
		auto userPublicKey = DataTypeConverter::hexToBinString(userPublicKeyHex);
		apolloTransactions.clear();
		model::TransactionsManager::TransactionList transactions;
		try {
			transactions = tm->getSortedTransactionsForUser(groupAlias, itUser->second);
		}
		catch (model::TransactionsManager::AccountInGroupNotFoundException& ex) {
			errorLog.error("couldn't find transactions for user: %s", ex.getFullString());
			continue;
		}
		select.execute();
		std::map<uint64_t, int> transactionPreviousIndexMap;
		if (apolloTransactions.size() + 1 != transactions.size()) {
			errorLog.error("transactions count for user: %u don't match", (unsigned)userId);
			return;
		}
		for (int i = 0; i < apolloTransactions.size(); i++) {
			auto transaction = &apolloTransactions[i];
			transactionPreviousIndexMap.insert({ transaction->get<1>(), i });
		}
		std::vector<TransactionTuple*> sortedTransactions;
		while (sortedTransactions.size() < apolloTransactions.size()) {
			if (!sortedTransactions.size()) {
				sortedTransactions.push_back(&apolloTransactions[0]);
			}
			else {
				auto it = transactionPreviousIndexMap.find(sortedTransactions.back()->get<0>());
				assert(it != transactionPreviousIndexMap.end());
				sortedTransactions.push_back(&apolloTransactions[it->second]);
			}
		}
		i = 0;
		calcBalance.reset();
		dbBalance.reset();
		dbDecay.reset();
		calcDecay.reset();

		Poco::DateTime lastBalanceDate;
		for (auto itTransaction = transactions.begin(); itTransaction != transactions.end(); itTransaction++) {
			auto transactionBody = (*itTransaction)->getTransactionBody();
			if (itTransaction == transactions.begin() && transactionBody->isRegisterAddress()) {
				lastBalanceDate = Poco::Timestamp((Poco::UInt64)transactionBody->getCreatedSeconds() * Poco::Timestamp::resolution());
				continue;
			}
			if (transactionBody->isRegisterAddress()) {
				errorLog.error("more than one register address transaction for user: %u", (unsigned)userId);
				return;
			}
			auto apolloTransaction = sortedTransactions[i];
			// calculate decay
			Poco::DateTime localDate = Poco::Timestamp((Poco::UInt64)transactionBody->getCreatedSeconds() * Poco::Timestamp::resolution());
			if (!apolloTransaction->get<5>().isNull()) {
				auto decayEnd = apolloTransaction->get<3>();
				auto decayStart = apolloTransaction->get<5>();
				if (!mpfr_zero_p((mpfr_ptr)calcBalance)) {
					//mpfr_set(calcDecay, (mpfr_ptr)calcBalance, gDefaultRound);
					calcDecay = calcBalance;
					calculateDecayFactorForDuration(decayForDuration, gDecayFactorGregorianCalender, lastBalanceDate, localDate);
					calculateDecayFast(decayForDuration, calcBalance);
					calcDecay = calcBalance - calcDecay;
					//mpfr_sub(calcDecay, calcBalance, calcDecay, gDefaultRound);
				}
			}
			// calculate balance
			std::string amountString;
			bool subtract = false;
			if (transactionBody->isCreation()) {
				auto creation = transactionBody->getCreationTransaction();
				amountString = creation->getAmount();
			}
			else if (transactionBody->isTransfer()) {
				auto transfer = transactionBody->getTransferTransaction();
				amountString = transfer->getAmount();
				if (transfer->getRecipientPublicKeyString() == *userPublicKey) {

				}
				else if (transfer->getSenderPublicKeyString() == *userPublicKey) {
					subtract = true;
				}
				else {
					assert(true || "transaction don't belong to us");
				}
			}
			else {
				errorLog.error("not expected transaction type");
				return;
			}
			if (mpfr_set_str(amount, amountString.data(), 10, gDefaultRound)) {
				throw model::gradido::TransactionValidationInvalidInputException("amount cannot be parsed to a number", "amount", "string");
			}
			if (!subtract) {
				mpfr_add(calcBalance, calcBalance, amount, gDefaultRound);
			}
			else {
				mpfr_sub(calcBalance, calcBalance, amount, gDefaultRound);
			}
			// compare with balance and decay from db
			dbBalance = apolloTransaction->get<2>();
			dbDecay = apolloTransaction->get<4>();

			bool notIdentical = false;
			diff = dbBalance - calcBalance;
			//mpfr_sub(diff, dbBalance, calcBalance, gDefaultRound);
			mpfr_abs(diff, diff, gDefaultRound);
			//0.0001
			if (mpfr_cmp_d(diff, 0.0002) > 0) {
			//if (mpfr_cmp_d(diff, 0.000000000000001) > 0)  {
				notIdentical = true;
			}
			else {
				diff = dbDecay - calcDecay;
				//mpfr_sub(diff, dbDecay, calcDecay, gDefaultRound);
				mpfr_abs(diff, diff, gDefaultRound);
				if (mpfr_cmp_d(diff, 0.0002) > 0) {
				//if (mpfr_cmp_d(diff, 0.000000000000001) > 0) {
					notIdentical = true;
				}
			}
			if (notIdentical && (dbBalance != calcBalance || dbDecay != calcDecay)) {
				std::string dbD;
				
				bool notIdentical = false;
				if (dbBalance != calcBalance) {
					errorLog.information("balance:\nnode: %s\ndb  : %s",
						calcBalance.toString(),
						dbBalance.toString()
					);
					notIdentical = true;
				}
				if (dbDecay != calcDecay) {
					errorLog.information("decay:\nnode: %s\ndb  : %s",
						calcDecay.toString(),
						dbDecay.toString()
					);
					notIdentical = true;
				}
				if (notIdentical) {
					auto createdTimeString = Poco::DateTimeFormatter::format(transactionBody->getCreated(), Poco::DateTimeFormat::SORTABLE_FORMAT);
					auto balanceDateString = Poco::DateTimeFormatter::format(apolloTransaction->get<3>(), Poco::DateTimeFormat::SORTABLE_FORMAT);
					errorLog.error("balance or decay are not identical for user: %u and transaction nr: %d (%u), balance date: %s, created time: %s",
						(unsigned)userId, i, (unsigned)apolloTransaction->get<0>(), balanceDateString, createdTimeString
					);
					
					std::string temp;
					const int roundStrings = 25;
					auto dbDecayStartTime = apolloTransaction->get<5>();
					// decayedBalance = balance * f ^ s
					// decayedBalance / balance = f ^ s
					// (decayedBalance / balance) ^(1.0 / s) = f
					Decimal dbAmount(apolloTransaction->get<6>());
					// checked
					Decimal prevDBBalance = dbBalance - dbAmount - dbDecay;
					prevDBBalance = prevDBBalance.toString(roundStrings);
					Decimal decayBalance = dbBalance - dbAmount;
					decayBalance = decayBalance.toString(roundStrings);
					Decimal dbF = decayBalance / prevDBBalance;
					Decimal decaySeconds((apolloTransaction->get<3>() - dbDecayStartTime).totalSeconds());
					Decimal nodeF1, nodeF2;
					mpfr_pow(nodeF1, apolloDecayFactor1, decaySeconds, gDefaultRound);
					mpfr_pow(nodeF2, apolloDecayFactor2, decaySeconds, gDefaultRound);
					nodeF1 = nodeF1.toString(roundStrings);
					nodeF2 = nodeF2.toString(roundStrings);
					Decimal cDecayBalance1 = prevDBBalance * nodeF1;
					Decimal cDecayBalance2 = prevDBBalance * nodeF2;
					auto prevBalanceString = prevDBBalance.toString(45);
					plugin::apollo::DecayDecimal prevDBBalanceApollo(prevDBBalance.toString(45));
					prevDBBalanceApollo.applyDecay((apolloTransaction->get<3>() - dbDecayStartTime));
					auto hajsa = prevDBBalanceApollo.toString(40);
					cDecayBalance1 = cDecayBalance1.toString(roundStrings);
					cDecayBalance2 = cDecayBalance2.toString(roundStrings);
					Decimal ccBalance1 = cDecayBalance1 + dbAmount;
					Decimal ccBalance2 = cDecayBalance2 + dbAmount;
					Decimal aBalance1 = prevDBBalanceApollo + dbAmount;
					ccBalance1 = ccBalance1.toString(roundStrings);
					ccBalance2 = ccBalance2.toString(roundStrings);
					Decimal ccDiff1 = ccBalance1 - dbBalance;
					Decimal ccDiff2 = ccBalance2 - dbBalance;
					Decimal aDiff1 = aBalance1 - dbBalance;
					Decimal fDiff1 = nodeF1 - dbF;
					Decimal fDiff2 = nodeF2 - dbF;
					Decimal powF(1);
					powF /= decaySeconds;
					// x = pow(y, 5)
					// y = pow(x, 1.0 / 5);
					// Y = Math.Log(x) / Math.Log(5)
					//mpfr_pow(f, f, powF, gDefaultRound);

					Decimal diffPerSecond(fDiff1);
					diffPerSecond /= decaySeconds;
					errorLog.error("  diff: %s, seconds: %s, fDiff: %s, fDiff2: %s", 
						diff.toString(40), decaySeconds.toString(40), fDiff1.toString(40), fDiff2.toString(40));
					errorLog.error("ccdiff1: %s, ccdiff2: %s, adiff1: %s", ccDiff1.toString(40), ccDiff2.toString(40), aDiff1.toString(40));
					
					//id, balance, decay, seconds, diff, cutDiff
					csvFile << apolloTransaction->get<0>() << ","
						<< calcBalance.toString(30) << ","
						<< calcDecay.toString(30) << ","
						<< decaySeconds.toString(30) << ","
						<< diff.toString(30) << ","
						<< ccDiff1.toString(30) << ","
						<< ccDiff2.toString(30) << ","
						<< fDiff1.toString(30) << ","
						<< fDiff2.toString(30) << ","
						<< aDiff1.toString(30)
						<< std::endl;
					//errorLog.error(tm->getUserTransactionsDebugString(groupAlias, userPublicKeyHex));
					//return;
				}
			}
			else {
				//printf("(%d) identical\n", (int)apolloTransaction.get<0>());
				mpfr_set(calcBalance, (mpfr_ptr)dbBalance, gDefaultRound);
			}
			lastBalanceDate = localDate;
			i++;
		}
	}
	/*
	int f = 1;
	for (int i = 0; i < 9; i++)
	{
		Decimal nodeF;
		mpfr_pow_si(nodeF, gDecayFactorGregorianCalender, f, gDefaultRound);
		errorLog.error("(%d): %s (%d)", i, nodeF.toString().substr(0,27), f);
		int f2 = 3;
		for (int j = 0; j < 10; j++) {
			Decimal decayed;
			mpfr_mul_si(decayed, nodeF, f2, gDefaultRound);
			errorLog.error("(%d.%d): %s", i, j, decayed.toString());
			f2 *= 7;
		}
		f *= 10;
	}

	*/
	speedLog.information("checkApolloServerDecay in %s", timeUsed.string());
	csvFile.close();
	mm->releaseMathMemory(amount);
	mm->releaseMathMemory(decayForDuration);
	unloadDefaultDecayFactors();
}

void GradidoBlockchainConnector::compareWithGradidoNode(const std::string& groupAlias, bool apollo/* = true*/)
{
	auto dbSession = ConnectionManager::getInstance()->getConnection();
	Poco::Logger& errorLog = Poco::Logger::get("errorLog");
	Poco::Logger& speedLog = Poco::Logger::get("speedLog");
	errorLog.information("starting compareWithGradidoNode");
	if (apollo) {

		// end balance
		Poco::Data::Statement select(dbSession);
		std::list<Poco::Tuple<uint64_t, std::string>> users;
		task::CPUTaskGroup<task::CompareApolloWithGradidoNode> compareTasks(100, false);
		select << "select id, hex(public_key) from users", into(users), now;		
		int userId = 0;
		std::string dbBalanceString;
		Poco::DateTime balanceDate;
		Profiler timeUsed;

		for (auto userTuple : users) {
			userId = userTuple.get<0>();
			if(userId != 11) continue;
			auto pubkeyHex = userTuple.get<1>();
			if(!pubkeyHex.size()) continue;
			Poco::AutoPtr<task::CompareApolloWithGradidoNode> task = new task::CompareApolloWithGradidoNode(userId, pubkeyHex, groupAlias);
			task->run();
			compareTasks.pushTask(task);
			
		}
		compareTasks.join();
		speedLog.information("used: %s for compare apollo transactions with gradido node", timeUsed.string());
		int countNotIdenticals = 0;
		int countIdenticals = 0;
		for (auto compareTask : compareTasks.getPendingTasksList()) {
			if (!compareTask->isIdentical()) {
				countNotIdenticals++;
				errorLog.error("transactions from user %u not identical: %s",
					(unsigned)compareTask->getUserId(), compareTask->getErrorString()
				);
			}
			else {
				countIdenticals++;
			}
		}
		errorLog.information("%d/%d identical",
			countIdenticals, countIdenticals + countNotIdenticals
		);
		
	}

}

void GradidoBlockchainConnector::sendArchivedTransactionsToGradidoNode(const std::string& groupAlias, bool iota /* = false */, bool apollo /* = false */)
{
	Poco::Logger& speedLog = Poco::Logger::get("speedLog");
	Poco::Logger& errorLog = Poco::Logger::get("errorLog");
	auto tm = model::TransactionsManager::getInstance();
	auto mm = MemoryManager::getInstance();

	// code for loading old transactions from db
	Poco::AutoPtr<model::import::LoginServer> loginServerImport;
	if (!apollo) {
		loginServerImport = new model::import::LoginServer;
	}
	else {
		loginServerImport = new model::import::ApolloServer(groupAlias);
	}
	try {
		loginServerImport->loadAll(groupAlias);
	}
	catch (GradidoBlockchainException& ex) {
		if (apollo) {
			printf("error by importing from apollo server: %s\n", ex.getFullString().data());
			return;
		}
		else {
			printf("error by importing from login server: %s\n", ex.getFullString().data());
		}
	}
	Profiler waitOnTransactions;
	if (!apollo) {
		Poco::AutoPtr<model::import::CommunityServer> communityServerImport = new model::import::CommunityServer;
		try {
			communityServerImport->loadAll(groupAlias, true, loginServerImport);
		}
		catch (GradidoBlockchainException& ex) {
			printf("error by importing from community server: %s\n", ex.getFullString().data());
		}
		
		while (!communityServerImport->isAllTransactionTasksFinished()) {
			Poco::Thread::sleep(10);
		}
		communityServerImport->cleanTransactions();
	}
	else {
		Profiler waitOnPrepareTransactions;
		auto apolloServer = (model::import::ApolloServer*)(loginServerImport.get());
		while (!apolloServer->isAllTransactionTasksFinished()) {
			Poco::Thread::sleep(10);
		}
		apolloServer->cleanTransactions();
		speedLog.information("waited on Apollo Prepare Transactions Tasks: %s", waitOnPrepareTransactions.string());
	}
	speedLog.information("wait for transactions: %s", waitOnTransactions.string());
	const model::TransactionsManager::TransactionList* transactions;
	try {
		transactions = tm->getSortedTransactions(groupAlias);
	}
	catch (model::TransactionsManager::MissingTransactionNrException& ex) {
		errorLog.error("hole in transactions: %s", ex.getFullString());
		return;
	}

	
	try {
		transactions = tm->getSortedTransactions(groupAlias);
	}
	catch (model::TransactionsManager::MissingTransactionNrException& ex) {
		errorLog.error("hole in transactions: %s", ex.getFullString());
		return;
	}

	int transactionNr = 1;
	auto pt = model::PendingTransactions::getInstance();
	auto lastTransactionNrOnGradidoNode = gradidoNodeRPC::getLastTransactionNr(groupAlias);
	if (iota) {
		Profiler sendTransactionViaIotaTime;		
		for (auto it = transactions->begin(); it != transactions->end(); it++) {
			if (transactionNr <= lastTransactionNrOnGradidoNode) {
				transactionNr++;
				continue;
			}
			auto transaction = (*it);
			auto transactionBody = transaction->getTransactionBody();
			auto serializedTransaction = transaction->getSerialized();
			auto transactionCopy = std::make_unique<model::gradido::GradidoTransaction>(serializedTransaction.get());
			auto gradidoBlock = model::gradido::GradidoBlock::create(
				std::move(transactionCopy),
				transactionNr,
				transactionBody->getCreatedSeconds(),
				nullptr
			);

			auto level = static_cast<model::gradido::TransactionValidationLevel>(
				model::gradido::TRANSACTION_VALIDATION_SINGLE |
				model::gradido::TRANSACTION_VALIDATION_DATE_RANGE |
				model::gradido::TRANSACTION_VALIDATION_SINGLE_PREVIOUS
				);

			model::TransactionsManagerBlockchain blockchain(groupAlias);
			transaction->validate(
				level,
				&blockchain,
				gradidoBlock
			);
			// provisional message id 
			auto provisional_message_id = model::table::TransactionClientDetail::calculateMessageId(transaction.get());
			auto transactionClientDetails = model::table::TransactionClientDetail::findByIotaMessageId(provisional_message_id);
			assert(transactionClientDetails.size());

			// send to iota
			// finale to hex for iota
			auto hex_message = DataTypeConverter::binToHex(std::move(serializedTransaction));

			std::string index = "GRADIDO." + groupAlias;
			Profiler powTime;
			auto iotaMessageId = ServerConfig::g_IotaRequestHandler->sendMessage(DataTypeConverter::binToHex(index), *hex_message);
			speedLog.information("pow time: %s", powTime.string());

			auto iotaMessageIdBin = DataTypeConverter::hexToBinString(iotaMessageId)->substr(0, 32);
			// update message id
			for (auto it = transactionClientDetails.begin(); it != transactionClientDetails.end(); ++it) {
				(*it)->setIotaMessageId(iotaMessageIdBin);
				(*it)->save();
			}

			pt->pushNewTransaction(std::move(model::PendingTransactions::PendingTransaction(
				iotaMessageId,
				transactionBody->getTransactionType(),
				"", transactionBody->getCreated(), 0
			)));			
			
			
			Profiler waitingOnIotaTime;
			// and now we wait...
			int timeout = 1000;
			
			while (timeout) {
				auto it = pt->findTransaction(iotaMessageId);
				if (it != pt->getPendingTransactionsEnd() && 
					it->state != model::PendingTransactions::PendingTransaction::State::SENDED) {
						break;					
				}
				Poco::Thread::sleep(50);
				timeout--;
			}
			speedLog.information("(%u) %s waited on iota", (unsigned)transactionNr, waitingOnIotaTime.string());
			if (!timeout) {
				errorLog.error("waited long enough exit");
				break;
			}
			if (pt->isRejected(iotaMessageId)) {
				break;
			}
			//*/
			transactionNr++;
		}
		speedLog.information("sended %u transaction in %s via iota", (unsigned)(transactionNr - lastTransactionNrOnGradidoNode), sendTransactionViaIotaTime.string());
		
	}
	else {
		Profiler timeUsed;

		std::list<task::TaskPtr> putTasks;
		MultithreadQueue<double> transactionRunningTimes;
		Profiler scheduleTime;
		for (auto it = transactions->begin(); it != transactions->end(); it++)
		{
			if (transactionNr <= lastTransactionNrOnGradidoNode) {
				transactionNr++;
				continue;
			}
			task::TaskPtr putTask = new task::SendTransactionToGradidoNode(
				*it, 
				transactionNr, 
				groupAlias, 
				&transactionRunningTimes,
				loginServerImport
			);
			putTasks.push_back(putTask);
			putTask->scheduleTask(putTask);
			transactionNr++;
		}
		speedLog.information("time for scheduling %d transactions: %s", transactionNr - 1, scheduleTime.string());
		Poco::Thread::sleep(15);
		printf("\n");
		// wait on finishing task, will keep running if only one error occur!
		printf("\rtransaction: %d/%d", transactions->size() - putTasks.size(), transactions->size());
		while (putTasks.size()) {
			for (auto it = putTasks.begin(); it != putTasks.end(); it++) {
				if ((*it)->isTaskFinished()) {
					it = putTasks.erase(it);
					printf("\rtransaction: %d/%d", transactions->size() - putTasks.size(), transactions->size());
					if (it == putTasks.end()) break;
				}
			}
			Poco::Thread::sleep(35);
		}
		putTasks.clear();

		printf("\rtransaction: %d/%d", transactions->size() - putTasks.size(), transactions->size());
		printf("\n");
		double runtimeSum = 0.0;
		double temp = 0.0;
		while (transactionRunningTimes.pop(temp)) {
			runtimeSum += temp;
		}
		speedLog.information("time used for sending %d transaction to gradido node: %s", transactionNr - 1, timeUsed.string());
		speedLog.information("time used for sending on gradido node: %f ms", runtimeSum);
		speedLog.information("time used per transaction: %f ms", runtimeSum / (double)transactions->size());
	}
	Profiler cleanUpTime; 
	mm->clearProtobufMemory();
	mm->clearMathMemory();
	mm->clearMemory();
	tm->clear();
	speedLog.information("time used for memory clean up: %s", cleanUpTime.string());
}

void GradidoBlockchainConnector::createConsoleFileAsyncLogger(std::string name, std::string filePath)
{
	Poco::AutoPtr<Poco::ConsoleChannel> logConsoleChannel(new Poco::ConsoleChannel);
	Poco::AutoPtr<Poco::FileChannel> logFileChannel(new Poco::FileChannel(filePath));
	logFileChannel->setProperty("rotation", "500 K");
	logFileChannel->setProperty("compress", "true");
	logFileChannel->setProperty("archive", "timestamp");
	Poco::AutoPtr<Poco::SplitterChannel> logSplitter(new Poco::SplitterChannel);
	logSplitter->addChannel(logConsoleChannel);
	logSplitter->addChannel(logFileChannel);

	Poco::AutoPtr<Poco::AsyncChannel> logAsyncChannel(new Poco::AsyncChannel(logSplitter));

	Poco::Logger& log = Poco::Logger::get(name);
	log.setChannel(logAsyncChannel);
	log.setLevel("information");
}

void GradidoBlockchainConnector::createFileLogger(std::string name, std::string filePath)
{
	Poco::AutoPtr<Poco::FileChannel> logFileChannel(new Poco::FileChannel(filePath));
	logFileChannel->setProperty("rotation", "500 K");
	logFileChannel->setProperty("archive", "timestamp");
	Poco::Logger& log = Poco::Logger::get(name);
	log.setChannel(logFileChannel);
	log.setLevel("information");
}

int GradidoBlockchainConnector::main(const std::vector<std::string>& args)
{
	Profiler usedTime;
	if (_helpRequested)
	{
		displayHelp();
	}
	else
	{
		// ********** logging ************************************
		std::string log_Path = "/var/log/gradido/";
//#ifdef _WIN32
#if defined(_WIN32) || defined(_WIN64)
		log_Path = "./";
#endif
		if (mConfigPath != "") {
			log_Path = "./";
		}

		// init speed logger
		Poco::AutoPtr<Poco::SimpleFileChannel> speedLogFileChannel(new Poco::SimpleFileChannel(log_Path + "speedLog.txt"));
		/*
		The optional log file rotation mode:
		never:      no rotation (default)
		<n>:  rotate if file size exceeds <n> bytes
		<n> K:     rotate if file size exceeds <n> Kilobytes
		<n> M:    rotate if file size exceeds <n> Megabytes
		*/
		
		// logging for request handling
		createConsoleFileAsyncLogger("requestLog", log_Path + "requestLog.txt");

		// error logging
		createConsoleFileAsyncLogger("errorLog", log_Path + "errorLog.txt");
		Poco::Logger& errorLog = Poco::Logger::get("errorLog");

		// speed logginh
		createConsoleFileAsyncLogger("speedLog", log_Path + "speedLog.txt");
		Poco::Logger& speedLog = Poco::Logger::get("speedLog");


		// *************** load from config ********************************************

		std::string cfg_Path = Poco::Path::home() + ".gradido/";
		try {
				if(mConfigPath != "") {
				    loadConfiguration(mConfigPath);
 				} else {
					loadConfiguration(cfg_Path + "blockchain_connector.properties");
                }
		}
		catch (Poco::Exception& ex) {
			errorLog.error("error loading config: %s", ex.displayText());
		}

		unsigned short json_port = (unsigned short)config().getInt("JSONServer.port", 1271);

		//printf("show mnemonic list: \n");
		//printf(ServerConfig::g_Mnemonic_WordLists[ServerConfig::MNEMONIC_BIP0039_SORTED_ORDER].getCompleteWordList().data());
		if (!ServerConfig::initServerCrypto(config())) {
			//printf("[Gradido_LoginServer::%s] error init server crypto\n", __FUNCTION__);
			errorLog.error("[Gradido_LoginServer::main] error init server crypto");
			return Application::EXIT_CONFIG;
		}

		if (!CryptoConfig::loadMnemonicWordLists()) {
			errorLog.error("[Gradido_LoginServer::main] error calling loadMnemonicWordLists");
			return Application::EXIT_CONFIG;
		}
		CryptoConfig::loadCryptoKeys(config());

		

		Poco::Net::initializeSSL();
		if(!ServerConfig::initSSLClientContext()) {
			//printf("[Gradido_LoginServer::%s] error init server SSL Client\n", __FUNCTION__);
			errorLog.error("[Gradido_LoginServer::main] error init server SSL Client");
			return Application::EXIT_CONFIG;
		}

		ServerConfig::initMysql(config());
		ServerConfig::initIota(config());
		ServerConfig::g_GradidoNodeUri = Poco::URI(config().getString("gradidoNode", "http://127.0.0.1:8340"));
		ServerConfig::readUnsecureFlags(config());

		uint8_t worker_count = Poco::Environment::processorCount()-2;
		//worker_count = 1;
		ServerConfig::g_WorkerThread = new task::CPUSheduler(worker_count, "grddWr");

		model::table::VersionsManager::getInstance()->migrate();
		
		// JSON Interface Server
		Poco::Net::ServerSocket json_svs(json_port);
		Poco::Net::HTTPServer json_srv(new JsonRequestHandlerFactory, json_svs, new Poco::Net::HTTPServerParams);
		// start the json server
		json_srv.start();
		speedLog.information("[GradidoBlockchainConnector::main] started in %s", usedTime.string());

		// livetest_1221
		// livetest_2112
		std::string groupAlias = "livetest_2112";
		if(mArchivedTransactionsHandling == ArchivedTransactionsHandling::USE_COMMUNITY_SERVER_DATA) {
			checkCommunityServerStateBalances(groupAlias);
		}
		if (mArchivedTransactionsHandling == ArchivedTransactionsHandling::USE_APOLLO_SERVER_DATA) {
			checkApolloServerDecay(groupAlias);
		}
		if (ArchivedTransactionsHandling::NONE != (ArchivedTransactionsHandling)(
			(int)mArchivedTransactionsHandling & ((int)ArchivedTransactionsHandling::SEND_DATA_TO_GRADIDO_NODE | (int)ArchivedTransactionsHandling::SEND_DATA_TO_IOTA)
		)){
			sendArchivedTransactionsToGradidoNode(groupAlias, 
				((int)mArchivedTransactionsHandling & (int)ArchivedTransactionsHandling::SEND_DATA_TO_IOTA) == (int)ArchivedTransactionsHandling::SEND_DATA_TO_IOTA,
				((int)mArchivedTransactionsHandling & (int)ArchivedTransactionsHandling::USE_APOLLO_SERVER_DATA) == (int)ArchivedTransactionsHandling::USE_APOLLO_SERVER_DATA
			);
		}
		if (ArchivedTransactionsHandling::NONE != (ArchivedTransactionsHandling)(
			(int)mArchivedTransactionsHandling & (int)ArchivedTransactionsHandling::COMPARE_DATA_WITH_GRADIDO_NODE
			)) {
			compareWithGradidoNode(groupAlias, 
				((int)mArchivedTransactionsHandling & (int)ArchivedTransactionsHandling::USE_APOLLO_SERVER_DATA) == (int)ArchivedTransactionsHandling::USE_APOLLO_SERVER_DATA
			);
		}

		// wait for CTRL-C or kill
		waitForTerminationRequest();

		// Stop the json server
		json_srv.stop();

		ServerConfig::g_WorkerThread->stop();

		ServerConfig::unload();
		Poco::Net::uninitializeSSL();
		// Optional:  Delete all global objects allocated by libprotobuf.
		google::protobuf::ShutdownProtobufLibrary();

	}
	return Application::EXIT_OK;
}
