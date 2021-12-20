#include "TestJsonHelper.h"

#include "gtest/gtest.h"
//#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "JSONInterface/JsonRequestHandler.h"

using namespace rapidjson;

namespace testHelper {
	std::string stringify(const rapidjson::Document& json)
	{
		StringBuffer buffer;
		PrettyWriter<StringBuffer> writer(buffer);
		json.Accept(writer);

		return buffer.GetString();
	}

	void logErrorDetails(const rapidjson::Document& json)
	{
		std::string state;
		JsonRequestHandler jsonCall;
		jsonCall.getStringParameter(json, "state", state);
		if (state != "success") {
			std::string msg;
			std::string details;
			jsonCall.getStringParameter(json, "msg", msg);
			jsonCall.getStringParameter(json, "details", details);
			std::clog << "msg: " << msg;
			if (details.size()) {
				std::clog << ", details: " << details;
			}
			std::clog << std::endl;
		}
	}
	void checkDetails(const rapidjson::Document& json, std::vector<std::string> expectedMessages)
	{
		auto detailsIt = json.FindMember("details");
		ASSERT_NE(detailsIt, json.MemberEnd());
		ASSERT_TRUE(detailsIt->value.IsArray());
		for (auto it = detailsIt->value.Begin(); it != detailsIt->value.End(); it++) {
			bool found = false;
			for (auto expectedIt = expectedMessages.begin(); expectedIt != expectedMessages.end(); expectedIt++) {
				ASSERT_TRUE(it->IsString());
				if (*expectedIt == it->GetString()) {
					found = true;
					break;
				}
			}
			ASSERT_TRUE(found);
		}
	}
}