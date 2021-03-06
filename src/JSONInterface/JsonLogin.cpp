#include "JsonLogin.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/crypto/SealedBoxes.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/crypto/CryptoConfig.h"
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

			std::string username, encryptedPasswordHex, groupAlias;
			auto mm = MemoryManager::getInstance();
			getStringParameter(rapidjson_params, "name", username);
			getStringParameter(rapidjson_params, "password", encryptedPasswordHex);
			getStringParameter(rapidjson_params, "groupAlias", groupAlias);
			if (!username.size() || !encryptedPasswordHex.size() || !groupAlias.size()) {
				throw HandleRequestException("parameter error");
			}

			if (!model::gradido::TransactionBase::isValidGroupAlias(groupAlias)) {
				throw HandleRequestException("invalid group alias");
			}

			auto encryptedPasswordBin = DataTypeConverter::hexToBin(encryptedPasswordHex);
			auto password = SealedBoxes::decrypt(ServerConfig::g_JwtPrivateKey, encryptedPasswordBin);
			auto jwtToken = SessionManager::getInstance()->login(username, password, groupAlias, mClientIp.toString());
			// don't seems to work
			response.set("token", jwtToken);

			mm->releaseMemory(encryptedPasswordBin);
			rapid_json_result = stateSuccess();
			rapid_json_result.AddMember("token", Value(jwtToken.data(), alloc), alloc);
		}
		catch (SealedBoxes::DecryptException& ex) {
			rapid_json_result = stateError("cannot decrypt password");
		}
		catch (model::InvalidPasswordException& ex) {
			rapid_json_result = stateError(ex.what());
		}		
		catch (CryptoConfig::MissingKeyException& ex) {
			std::string errorMessage = ex.getKeyName() + " is missing from config";
			rapid_json_result = stateError(errorMessage.data());
		}
		catch (DecryptionFailedException& ex) {
			rapid_json_result = stateError("Internal Server Error");
			Poco::Logger::get("errorLog").critical("Decryption failed: %s", ex.getFullString());
		}
		catch (GradidoBlockchainException& ex) {
			rapid_json_result = stateError(ex.what());
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
