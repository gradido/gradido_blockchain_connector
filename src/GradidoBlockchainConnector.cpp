#include "GradidoBlockchainConnector.h"
#include "ServerConfig.h"
#include "JSONInterface/JsonRequestHandlerFactory.h"
#include "model/table/VersionsManager.h"

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
#include "model/import/LoginServer.h"
#include "model/import/CommunityServer.h"
#include "gradido_blockchain/lib/Decay.h"

GradidoBlockchainConnector::GradidoBlockchainConnector()
	: _helpRequested(false), _checkCommunityServerStateBalancesRequested(false)
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
		.repeatable(false));
	options.addOption(
		Poco::Util::Option("config", "c", "use non default config file (default is ~/.gradido/blockchain_connector.properties)", false)
		.repeatable(false)
		.argument("Gradido_LoginServer.properties", true)
		.callback(Poco::Util::OptionCallback<GradidoBlockchainConnector>(this, &GradidoBlockchainConnector::handleOption)));
	// on windows use / instead of - or -- for options!
	options.addOption(
		Poco::Util::Option("checkBackup", "cb", "check db backup contain valid data, argument is backup type: community for backup in login- and community server format", false)
		.repeatable(false) // TODO: make repeatable if more than one type is supported
		.argument("community", true)
		.callback(Poco::Util::OptionCallback<GradidoBlockchainConnector>(this, &GradidoBlockchainConnector::handleOption)));
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

void GradidoBlockchainConnector::checkCommunityServerStateBalances()
{
	Poco::Logger& speedLog = Poco::Logger::get("speedLog");
	Poco::Logger& errorLog = Poco::Logger::get("errorLog");
	std::string groupAlias = "gdd1";
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

		model::table::VersionsManager::getInstance()->migrate();
		
		// JSON Interface Server
		Poco::Net::ServerSocket json_svs(json_port);
		Poco::Net::HTTPServer json_srv(new JsonRequestHandlerFactory, json_svs, new Poco::Net::HTTPServerParams);
		// start the json server
		json_srv.start();
		speedLog.information("[GradidoBlockchainConnector::main] started in %s", usedTime.string());

		if (_checkCommunityServerStateBalancesRequested) {
			checkCommunityServerStateBalances();
		}

		// wait for CTRL-C or kill
		waitForTerminationRequest();

		// Stop the json server
		json_srv.stop();

		ServerConfig::unload();
		Poco::Net::uninitializeSSL();
		// Optional:  Delete all global objects allocated by libprotobuf.
		google::protobuf::ShutdownProtobufLibrary();

	}
	return Application::EXIT_OK;
}
