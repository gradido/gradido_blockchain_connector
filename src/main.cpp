#include "GradidoBlockchainConnector.h"
#include <sodium.h>

#include "gradido_blockchain/lib/Profiler.h"
#include "ServerConfig.h"

#include "Poco/DateTimeParser.h"
#include "Poco/DateTimeFormatter.h"

#ifndef _TEST_BUILD


int main(int argc, char** argv)
{
	if (sodium_init() < 0) {
		/* panic! the library couldn't be initialized, it is not safe to use */
		printf("error initializing sodium, early exit\n");
		return -1;
	}

	std::string dateTimeString = __DATE__;
	//printf("Building date time string: %s\n", dateTimeString.data());
	std::string formatString("%b %d %Y");
	int timeZone = 0;

	Poco::DateTime buildDateTime = Poco::DateTimeParser::parse(formatString, dateTimeString, timeZone);
	ServerConfig::g_versionString = Poco::DateTimeFormatter::format(buildDateTime, "0.%y.%m.%d");
	//ServerConfig::g_versionString = "0.20.KW13.02";
	printf("Version: %s\n", ServerConfig::g_versionString.data());


	GradidoBlockchainConnector app;
	try {
		auto result = app.run(argc, argv);
		return result;
	}
	catch (Poco::Exception& ex) {
		printf("[Gradido_LoginServer::main] exception by starting server: %s\n", ex.displayText().data());
	}
	return -1;

}
#endif
