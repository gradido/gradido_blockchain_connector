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
	Document rapidjson_params;
	
	if (method == "POST") {
		// extract parameter from request
		parseJsonWithErrorPrintFile(request_stream, rapidjson_params);

		try {
			std::string username, encryptedPasswordHex;
			auto mm = MemoryManager::getInstance();
			getStringParameter(rapidjson_params, "name", username);
			getStringParameter(rapidjson_params, "password", encryptedPasswordHex);
			if (!username.size() || !encryptedPasswordHex.size()) {
				throw HandleRequestException("parameter error");
			}

			auto encryptedPasswordBin = DataTypeConverter::hexToBin(encryptedPasswordHex);
			AuthenticatedEncryption keyPair(ServerConfig::g_JwtPrivateKey);
			auto password = SealedBoxes::decrypt(&keyPair, encryptedPasswordBin);

			auto jwtToken = SessionManager::getInstance()->login(username, password, mClientIp.toString());
			response.set("token", jwtToken);

			mm->releaseMemory(encryptedPasswordBin);
			rapid_json_result = stateSuccess();
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
