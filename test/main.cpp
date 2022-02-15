
#include "main.h"
#include <list>
#include "gtest/gtest.h"

#include "Poco/Util/PropertyFileConfiguration.h"
#include "Poco/Util/LayeredConfiguration.h"
#include "Poco/Logger.h"
#include "Poco/Environment.h"
#include "Poco/Path.h"
#include "Poco/AsyncChannel.h"
#include "Poco/SimpleFileChannel.h"
#include "Poco/FileChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/SplitterChannel.h"

#include "gradido_blockchain/lib/Profiler.h"
#include "ServerConfig.h"



std::list<Test*> gTests;

void fillTests()
{
	gTests.push_back(new TestRegExp());
	//	gTests.push_back(new LoginTest());
}

int load(int argc, char* argv[]) {
	// init server config, init seed array
	std::clog << "[load]" << std::endl;
	Poco::AutoPtr<Poco::Util::LayeredConfiguration> test_config(new Poco::Util::LayeredConfiguration);
	std::string config_file_name = Poco::Path::home() + ".gradido/gradido_test.properties";
/*#ifdef WIN32 
	config_file_name = "GradidoBlockchainConnector.properties";
#endif*/
	if(argc > 1 && strlen(argv[1]) > 4) {
		config_file_name = argv[1];
	}

	try {
		auto cfg = new Poco::Util::PropertyFileConfiguration(config_file_name);
		test_config->add(cfg);
	}
	catch (Poco::Exception& ex) {
		std::clog << "[load] warning loading ~/.gradido/gradido_test.properties don't work, make sure this file exist! " 
			<< ex.displayText().data()
			<< std::endl;
		//return -3;
	}

	if (!ServerConfig::initServerCrypto(*test_config)) {
		//printf("[Gradido_LoginServer::%s] error init server crypto\n", __FUNCTION__);
		std::clog << "[load] error init server crypto" << std::endl;
		return -1;
	}
	
	
	std::string log_Path = "/var/log/gradido/";
//#ifdef _WIN32
#if defined(_WIN32) || defined(_WIN64)
	log_Path = "./";
#endif
	
	std::string filePath = log_Path + "errorLog.txt";
	Poco::AutoPtr<Poco::ConsoleChannel> logConsoleChannel(new Poco::ConsoleChannel);
	Poco::AutoPtr<Poco::FileChannel> logFileChannel(new Poco::FileChannel(filePath));
	Poco::AutoPtr<Poco::SplitterChannel> logSplitter(new Poco::SplitterChannel);
	logSplitter->addChannel(logConsoleChannel);
	logSplitter->addChannel(logFileChannel);

	Poco::AutoPtr<Poco::AsyncChannel> logAsyncChannel(new Poco::AsyncChannel(logSplitter));

	Poco::Logger& log = Poco::Logger::get("errorLog");
	log.setChannel(logAsyncChannel);
	log.setLevel("information");

	log.error("Test Error");

	Profiler timeUsed;

	fillTests();
	for (std::list<Test*>::iterator it = gTests.begin(); it != gTests.end(); it++)
	{
		std::clog << "call init on test: " << (*it)->getName() << std::endl;
		if ((*it)->init()) printf("Fehler bei Init test: %s\n", (*it)->getName());
	}
	return 0;
}

int run()
{
	std::clog << "[GradidoBlockchainConnectorTest::run]" << std::endl;
	for (std::list<Test*>::iterator it = gTests.begin(); it != gTests.end(); it++)
	{
		std::string name = (*it)->getName();
		std::clog << "running test:  " << name << std::endl;
		try {
			if (!(*it)->test()) std::clog << "Success" << std::endl;
		} catch(std::exception& ex) {
			std::clog << "exception in running test: " << ex.what() << std::endl;
		}
	}
	return 0;
}

void endegTests()
{
	for (std::list<Test*>::iterator it = gTests.begin(); it != gTests.end(); it++)
	{
		if (*it) {
			delete *it;
		}

	}
	gTests.clear();
	
}


int main(int argc, char** argv)
{
	try {
		if (load(argc, argv) < 0) {
			std::clog << "early exit" << std::endl;
			return -42;
		}
	} catch(std::exception& ex) {
		std::string exception_text = ex.what();
		std::clog << "no catched exception while loading: " << exception_text << std::endl;
	}
	
  	//printf ("\nStack Limit = %ld and %ld max\n", limit.rlim_cur, limit.rlim_max);

	run();
	endegTests();

	::testing::InitGoogleTest(&argc, argv);
	auto result = RUN_ALL_TESTS();

	ServerConfig::unload();
	
	return result;
}
