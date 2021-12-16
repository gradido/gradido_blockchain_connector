#include "JsonSendTransactionIota.h"
#include "model/gradido/Transaction.h"
#include "lib/DataTypeConverter.h"

using namespace rapidjson;

/*
{
	"bodyBytesBase64": "CAEStwEKZgpkCiDs2zemYO1PxD1Odwh5YxyUmDp+lyxgVmiQgiFdPLUahRJAvyYRVASJNvyYiTAT2D8t6QtVgekqnsPIJRAx6jG8tEqxdzwKGg/Jm0gatdY1Ix7DGbHMBRw/9CtoXQXueqqDChJNChBBR0UgT2t0b2JlciAyMDIxEgYIuMWpjQY6MQonCiAdkWfcgRcDfCIg+GbikK6U9Fp4WMTGtAxF7RRdvhisSxCA2sQJGgYIgJ/ZigYaBgjGxamNBiABKiCijBRel5hudg5iZqfeQxjzIMhnOJA+tmHmloMVW+snjTIEyWETAA==",
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
	auto paramError = getStringParameter(params, "bodyBytesBase64", bodyBytesBase64String);
	if (paramError.IsObject()) { return paramError; }

	paramError = getStringParameter(params, "groupAlias", groupAlias);
	if (paramError.IsObject()) { return paramError; }

	auto signaturePairsIt = params.FindMember("signaturePairs");
	if (signaturePairsIt == params.MemberEnd()) {
		return stateError("signaturePairs not found");
	}
	if (!signaturePairsIt->value.IsArray()) {
		return stateError("signaturePairs isn't a array");
	}
	
	auto transactionBody = model::gradido::TransactionBody::load(DataTypeConverter::base64ToBinString(bodyBytesBase64String));
	Poco::AutoPtr<model::gradido::Transaction> transaction(new model::gradido::Transaction(transactionBody));
	auto mm = MemoryManager::getInstance();

	for (auto it = signaturePairsIt->value.Begin(); it != signaturePairsIt->value.End(); it++) {
		std::string pubkeyHexString, signatureHexString;
		paramError = getStringParameter(*it, "pubkey", pubkeyHexString);
		if (paramError.IsObject()) { return paramError; }

		paramError = getStringParameter(*it, "signature", signatureHexString);
		if (paramError.IsObject()) { return paramError; }

		auto pubkeyBin = DataTypeConverter::hexToBin(pubkeyHexString);
		auto signatureBin = DataTypeConverter::hexToBin(signatureHexString);
		auto result = transaction->addSign(pubkeyBin, signatureBin);
		if (!result) {
			return stateError("error adding signature", transaction);
		}
		mm->releaseMemory(pubkeyBin);
		mm->releaseMemory(signatureBin);
	}
	if (transaction->getSignCount() < transactionBody->getTransactionBase()->getMinSignatureCount()) {
		return stateError("missing signatures");
	}
	if (1 != transaction->runSendTransaction(groupAlias)) {
		return stateError("error sending transaction", transaction);
	}
	auto response = stateSuccess();
	auto alloc = response.GetAllocator();
	response.AddMember("iotaMessageId", Value(transaction->getIotaMessageId().data(), alloc), alloc);
	return response;
}