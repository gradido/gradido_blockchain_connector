#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_JSON_INTERFACE_TEST_JSON_HELPER
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_JSON_INTERFACE_TEST_JSON_HELPER

#include "Poco/DateTime.h"

#include "rapidjson/document.h"

#include <string>
#include <vector>

namespace testHelper {
	std::string stringify(const rapidjson::Document& json);
	void logErrorDetails(const rapidjson::Document& json);
	void checkDetails(const rapidjson::Document& json, std::vector<std::string> expectedMessages);
	Poco::DateTime subtractMonthFromDate(Poco::DateTime date, int subMonth);
	Poco::DateTime addMonthToDate(Poco::DateTime date, int addMonth);
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_JSON_INTERFACE_TEST_JSON_HELPER