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

#include "iota_rust_clib.h"

GradidoBlockchainConnector::GradidoBlockchainConnector()
	: _helpRequested(false)
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

	/*options.addOption(
		Poco::Util::Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false));*/
	options.addOption(
		Poco::Util::Option("config", "c", "use non default config file (default is ~/.gradido/blockchain_connector.properties)", false)
		.repeatable(false)
		.argument("Gradido_LoginServer.properties", true)
		.callback(Poco::Util::OptionCallback<GradidoBlockchainConnector>(this, &GradidoBlockchainConnector::handleOption)));

}

void GradidoBlockchainConnector::handleOption(const std::string& name, const std::string& value)
{
	//printf("handle option: %s with value: %s\n", name.data(), value.data());
	if (name == "config") {
		mConfigPath = value;
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
		ServerConfig::readUnsecureFlags(config());

		model::table::VersionsManager::getInstance()->migrate();
		
		// JSON Interface Server
		Poco::Net::ServerSocket json_svs(json_port);
		Poco::Net::HTTPServer json_srv(new JsonRequestHandlerFactory, json_svs, new Poco::Net::HTTPServerParams);
		// start the json server
		json_srv.start();

		std::clog << "[GradidoBlockchainConnector::main] started in " << usedTime.string().data() << std::endl;

		std::clog << "try out iota" << std::endl;
		auto uri = ServerConfig::g_IotaRequestHandler->getRequestUri();
		auto iotaUrl = "http://" + uri.getHost() + ":" + std::to_string(uri.getPort());
		//iotaUrl = "https://api.lb-0.h.chrysalis-devnet.iota.cafe";
		std::clog << "iota: " << iotaUrl << std::endl;
		auto result = iota_send_indiced_transaction(
			iotaUrl.data(),
			"GRADIDO", 
			"Message"
		);
		printf("return with: %s\n", result);
		free_rust_string(result);

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
