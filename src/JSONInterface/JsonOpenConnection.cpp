#include "JsonOpenConnection.h"

#include "ServerConfig.h"
#include "SessionManager.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/crypto/KeyPairEd25519.h"
#include "gradido_blockchain/MemoryManager.h"

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

#include "Poco/JWT/Signer.h"
#include "Poco/JWT/JWTException.h"

using namespace rapidjson;

void JsonOpenConnection::handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response)
{
    mClientIp = request.clientAddress().host();
    std::istream& request_stream = request.stream();
	Document rapid_json_result;
	auto alloc = rapid_json_result.GetAllocator();
	Document rapidjson_params;
    auto method = request.getMethod();
    auto mm = MemoryManager::getInstance();

    std::string communityPubkeyHex; //community-key-A
    std::string signatureHex;

    if(method == "POST") {
        try {
			// extract parameter from request
			parseJsonWithErrorPrintFile(request_stream, rapidjson_params);
            getStringParameter(rapidjson_params, "community-key-A", communityPubkeyHex);
            getStringParameter(rapidjson_params, "signature", signatureHex);
            if (!communityPubkeyHex.size() || !signatureHex.size()) {
				throw HandleRequestException("parameter error");
			}

            auto communityPubkey = DataTypeConverter::hexToBin(communityPubkeyHex);
            auto signature = DataTypeConverter::hexToBin(signatureHex);

            KeyPairEd25519 callerKeyPair(communityPubkey->data());
            auto serverPubkey = mm->getMemory(crypto_sign_PUBLICKEYBYTES);
            crypto_sign_ed25519_sk_to_pk(*serverPubkey, *ServerConfig::g_JwtPrivateKey);
            auto serverPubkeyHex = serverPubkey->convertToHexString();
            if(!callerKeyPair.verify(serverPubkey, signature)) {
                throw Ed25519VerifyException("cannot verify signature", serverPubkeyHex, signatureHex);
            } 

            mm->releaseMemory(communityPubkey);
            mm->releaseMemory(signature);
            mm->releaseMemory(serverPubkey);

            // create connection jwt token
            Poco::Timestamp now;
            Poco::JWT::Token token;
            token.setType("JWT");
            token.setSubject("openConnection");
            token.setIssuedAt(now);
            token.setExpiration(now + ServerConfig::g_SessionValidDuration);

	        Poco::JWT::Signer signer(ServerConfig::g_JwtVerifySecret);
            auto serializedJWTToken = std::move(signer.sign(token, Poco::JWT::Signer::ALGO_HS256));
            rapid_json_result = stateSuccess();
			rapid_json_result.AddMember("token", Value(serializedJWTToken.data(), alloc), alloc);

        } catch(Ed25519VerifyException& ex) {
            rapid_json_result = stateError(ex.what(), ex);
            Poco::Logger::get("errorLog").debug("Ed25519VerifyException in JsonOpenConnection: %s", ex.getFullString());   
        } catch (GradidoBlockchainException& ex) {
            printf("GradidoBlockchainException: %s\n", ex.what());
            rapid_json_result = stateError(ex.what());
        } catch (Poco::Exception& ex) {
            rapid_json_result = stateError("Internal Server Error");
            Poco::Logger::get("errorLog").critical("Poco Exception in JsonOpenConnection: %s", ex.displayText());
        } catch (std::runtime_error& ex) {
            rapid_json_result = stateError("Internal Server Error");
            Poco::Logger::get("errorLog").critical("runtime error in JsonOpenConnection: %s", std::string(ex.what()));
        }
    }

    responseWithJson(rapid_json_result, request, response);
}