#include "JsonSendTransactionIota.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoTransaction.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

#include "ServerConfig.h"

using namespace rapidjson;

/*
{
	"bodyBytesBase64": "CAEStwEKZgpkCiDs2zemYO1PxD1Odwh5YxyUmDp+lyxgVmiQgiFdPLUahRJAvyYRVASJNvyYiTAT2D8t6QtVgekqnsPIJRAx6jG8tEqxdzwKGg/Jm0gatdY1Ix7DGbHMBRw/9CtoXQXueqqDChJNChBBR0UgT2t0b2JlciAyMDIxEgYIuMWpjQY6MQonCiAdkWfcgRcDfCIg+GbikK6U9Fp4WMTGtAxF7RRdvhisSxCA2sQJGgYIgJ/ZigYaBgjGxamNBiABKiCijBRel5hudg5iZqfeQxjzIMhnOJA+tmHmloMVW+snjTIEyWETAA==",
	"apolloTransactionId": 10,
	"signaturePairs": [
		{
			"pubkey": "ecdb37a660ed4fc43d4e770879631c94983a7e972c6056689082215d3cb51a85",
			"signature": "c1b2e8077c206bf78aeaefbdcdfe8a5ae32ddf9adca95ba1f5a93e885b7b3a00f224fe1fb0ea491d20966f7336fd90479c792432dc94b8c8b83dd00510fca508"
		}
	],
	"groupAlias":"gdd1"
}
*/

Document JsonSendTransactionIota::handle(const Document& params)
{
	std::string bodyBytesBase64String, groupAlias;
	uint64_t apolloTransactionId = 0;

	auto paramError = getStringParameter(params, "bodyBytesBase64", bodyBytesBase64String);
	if (paramError.IsObject()) { return paramError; }

	paramError = getStringParameter(params, "groupAlias", groupAlias);
	getUInt64Parameter(params, "apolloTransactionId", apolloTransactionId);
	//if (paramError.IsObject()) { return paramError; }

	auto signaturePairsIt = params.FindMember("signaturePairs");
	if (signaturePairsIt == params.MemberEnd()) {
		return stateError("signaturePairs not found");
	}
	if (!signaturePairsIt->value.IsArray()) {
		return stateError("signaturePairs isn't a array");
	}

	auto transactionBody = model::gradido::TransactionBody::load(DataTypeConverter::base64ToBinString(bodyBytesBase64String), ProtobufArenaMemory::create());
	std::unique_ptr<model::gradido::GradidoTransaction> transaction(new model::gradido::GradidoTransaction(transactionBody));
	auto mm = MemoryManager::getInstance();


	for (auto it = signaturePairsIt->value.Begin(); it != signaturePairsIt->value.End(); it++) {
		std::string pubkeyHexString, signatureHexString;
		paramError = getStringParameter(*it, "pubkey", pubkeyHexString);
		if (paramError.IsObject()) { return paramError; }

		paramError = getStringParameter(*it, "signature", signatureHexString);
		if (paramError.IsObject()) { return paramError; }

		auto pubkeyBin = DataTypeConverter::hexToBin(pubkeyHexString);
		auto signatureBin = DataTypeConverter::hexToBin(signatureHexString);
		try {
			transaction->addSign(pubkeyBin, signatureBin);
		}
		catch (...) {
			mm->releaseMemory(pubkeyBin);
			mm->releaseMemory(signatureBin);
			throw;
		}
		mm->releaseMemory(pubkeyBin);
		mm->releaseMemory(signatureBin);

	}

	transaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
	if (transaction->getSignCount() < transactionBody->getTransactionBase()->getMinSignatureCount()) {
		return stateError("missing signatures");
	}

	if (!model::gradido::TransactionBase::isValidGroupAlias(groupAlias)) {
		return stateError("invalid group alias");
	}
	// send transaction to iota
	auto raw_message = transaction->getSerialized();

	// finale to hex for iota
	auto hex_message = DataTypeConverter::binToHex(std::move(raw_message));

	std::string index = "GRADIDO." + groupAlias;

	//auto message_id = ServerConfig::g_IotaRequestHandler->sendMessage(DataTypeConverter::binToHex(index), *hex_message);

	auto response = stateSuccess();
	auto alloc = response.GetAllocator();
	//response.AddMember("iotaMessageId", Value(message_id.data(), message_id.size(), alloc), alloc);
	return response;
}
