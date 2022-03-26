#include "MockIotaHttpClientSession.h"

#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include <sodium.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

MockIotaHttpClientSession::MockIotaHttpClientSession()
	: mResponseStream(mResponseStringbuf.rdbuf()), mBuffer(mRequestStringbuf.rdbuf())
{

}

std::ostream& MockIotaHttpClientSession::sendRequest(Poco::Net::HTTPRequest& request)
{
	auto uri = request.getURI();
	Document responseJson(kObjectType);
	auto mm = MemoryManager::getInstance();
	auto alloc = responseJson.GetAllocator();

	Value data(kObjectType);
	
	auto randomMessageId = mm->getMemory(32);
	if (uri == "tips") {
		/*
		http://api.lb-0.h.chrysalis-devnet.iota.cafe/api/v1/tips
		{
			"data": {
				"tipMessageIds": [
					"2388bd6a4c26284889fe9150f3d50e60fd8fe0d09bfbd75dcb90a092b505e27b",
					"4dcfea575acd2e10634134f47ec93e0c34b2bdf32911ed43b304b0744c1abcb4",
					"b5ea4037d48841ff8593852761c5c32471743f8d5fceb7ca052481cbbf5fa285",
					"d1aed9c20deddb307827ae71e57e131c77c15ede8b65897eae9817a53199b4db"
				]
			}
		}
		*/
		Value tipMessageIds(kArrayType);
		for (int i = 0; i < 4; i++) {
			// recycle memory
			randombytes_buf(*randomMessageId, 32);
			tipMessageIds.PushBack(Value(DataTypeConverter::binToHex(randomMessageId).data(), alloc), alloc);
		}
		data.AddMember("tipMessageIds", tipMessageIds, alloc);
				
	}
	else if (uri == "messages") {
		/*
		http://api.lb-0.h.chrysalis-devnet.iota.cafe/api/v1/messages
		{
			"data": {
				"messageId": "2388bd6a4c26284889fe9150f3d50e60fd8fe0d09bfbd75dcb90a092b505e27b"
			}
		}
		*/
		randombytes_buf(*randomMessageId, 32);
		data.AddMember("messageId", Value(DataTypeConverter::binToHex(randomMessageId).data(), alloc), alloc);		
	}
	responseJson.AddMember("data", data, alloc);
	mm->releaseMemory(randomMessageId);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	responseJson.Accept(writer);
	mResponseStringbuf << std::string(buffer.GetString(), buffer.GetSize());

	return mBuffer;
}

std::istream& MockIotaHttpClientSession::receiveResponse(Poco::Net::HTTPResponse& response)
{
	return mResponseStream;
}