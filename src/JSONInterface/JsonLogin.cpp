#include "JsonLogin.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/crypto/SealedBoxes.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "ServerConfig.h"
#include "SessionManager.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

using namespace rapidjson;

void JsonLogin::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
	mClientIp = request.clientAddress().host();

	auto method = request.getMethod();
	std::istream& request_stream = request.stream();
	Document rapid_json_result;
	auto alloc = rapid_json_result.GetAllocator();
	Document rapidjson_params;
	
	if (method == "POST") {
	
		try {
			// extract parameter from request
			parseJsonWithErrorPrintFile(request_stream, rapidjson_params);

			std::string username, encryptedPasswordHex;
			auto mm = MemoryManager::getInstance();
			getStringParameter(rapidjson_params, "name", username);
			getStringParameter(rapidjson_params, "password", encryptedPasswordHex);
			if (!username.size() || !encryptedPasswordHex.size()) {
				throw HandleRequestException("parameter error");
			}

			auto encryptedPasswordBin = DataTypeConverter::hexToBin(encryptedPasswordHex);
			auto password = SealedBoxes::decrypt(ServerConfig::g_JwtPrivateKey, encryptedPasswordBin);
			auto jwtToken = SessionManager::getInstance()->login(username, password, mClientIp.toString());
			// don't seems to work
			response.set("token", jwtToken);

			mm->releaseMemory(encryptedPasswordBin);
			rapid_json_result = stateSuccess();
			rapid_json_result.AddMember("token", Value(jwtToken.data(), alloc), alloc);
		}
		catch (GradidoBlockchainException& ex)
		{
			rapid_json_result = stateError(ex.getFullString().data());
		}
		catch (Poco::Exception& ex) {
			rapid_json_result = stateError("Internal Server Error");
			Poco::Logger::get("errorLog").critical("Poco Exception in JsonLogin: %s", ex.displayText());
		}
		catch (std::runtime_error& ex) {
			rapid_json_result = stateError("Internal Server Error");
			Poco::Logger::get("errorLog").critical("runtime error in JsonLogin: %s", std::string(ex.what()));
		}
	}

	responseWithJson(rapid_json_result, request, response);
}
