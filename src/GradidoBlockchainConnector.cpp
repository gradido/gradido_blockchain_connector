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
#include "JSONInterface/JsonTransaction.h"
#include "model/import/LoginServer.h"
#include "model/import/CommunityServer.h"
#include "model/PendingTransactions.h"
#include "gradido_blockchain/lib/Decay.h"
#include "task/SendTransactionToGradidoNode.h"

GradidoBlockchainConnector::GradidoBlockchainConnector()
	: _helpRequested(false), _checkCommunityServerStateBalancesRequested(false), 
	_sendCommunityServerTransactionsToGradidoNodeRequested(false), _sendCommunityServerTransactionsToIotaRequested(false)
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
		Poco::Util::Option("checkBackup", "cb", "check db backup contain valid data, argument is backup type: community for backup in login- and community server format", false)
		.repeatable(false) // TODO: make repeatable if more than one type is supported
		.argument("community", true)
	);

	options.addOption(
		Poco::Util::Option("sendBackup", "sb", "send db backup transactions to gradidoNode or iota")
		.repeatable(false)
		.argument("gradidoNode", true)
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
			_checkCommunityServerStateBalancesRequested = true;
		}
		return;
	}
	else if (name == "sendBackup") {
		if (value == "gradidoNode") {
			_sendCommunityServerTransactionsToGradidoNodeRequested = true;
		}
		else if (value == "iota") {
			_sendCommunityServerTransactionsToIotaRequested = true;
		}
		return;
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

void GradidoBlockchainConnector::sendCommunityServerTransactionsToGradidoNode(const std::string& groupAlias, bool iota /* = false */)
{
	Poco::Logger& speedLog = Poco::Logger::get("speedLog");
	Poco::Logger& errorLog = Poco::Logger::get("errorLog");
	auto tm = model::TransactionsManager::getInstance();
	auto mm = MemoryManager::getInstance();

	// code for loading old transactions from db
	Poco::AutoPtr<model::import::LoginServer> loginServerImport = new model::import::LoginServer;
	try {
		loginServerImport->loadAll(groupAlias);
	}
	catch (GradidoBlockchainException& ex) {
		printf("error by importing from login server: %s\n", ex.getFullString().data());
	}
	Poco::AutoPtr<model::import::CommunityServer> communityServerImport = new model::import::CommunityServer;
	try {
		communityServerImport->loadAll(groupAlias, true, loginServerImport);
	}
	catch (GradidoBlockchainException& ex) {
		printf("error by importing from community server: %s\n", ex.getFullString().data());
	}
	Profiler waitOnTransactions;
	while (!communityServerImport->isAllTransactionTasksFinished()) {
		Poco::Thread::sleep(10);
	}
	communityServerImport->cleanTransactions();
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
	if (iota) {
		Profiler sendTransactionViaIotaTime;
		auto lastTransactionNrOnGradidoNode = gradidoNodeRPC::getLastTransactionNr(groupAlias);
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
			// send to iota
			// finale to hex for iota
			auto hex_message = DataTypeConverter::binToHex(std::move(serializedTransaction));

			std::string index = "GRADIDO." + groupAlias;
			auto iotaMessageId = ServerConfig::g_IotaRequestHandler->sendMessage(DataTypeConverter::binToHex(index), *hex_message);
			Profiler powTime;
			pt->pushNewTransaction(std::move(model::PendingTransactions::PendingTransaction(
				iotaMessageId,
				transactionBody->getTransactionType(),
				"", transactionBody->getCreated(), 0
			)));
			speedLog.information("pow time: %s", powTime.string());
			
			
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
			task::TaskPtr putTask = new task::SendTransactionToGradidoNode(*it, transactionNr, groupAlias, &transactionRunningTimes);
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

		std::string groupAlias = "gdd1";
		if (_checkCommunityServerStateBalancesRequested) {
			checkCommunityServerStateBalances(groupAlias);
		}
		if (_sendCommunityServerTransactionsToGradidoNodeRequested || _sendCommunityServerTransactionsToIotaRequested) {
			sendCommunityServerTransactionsToGradidoNode(groupAlias, _sendCommunityServerTransactionsToIotaRequested);
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
