#include "JsonRegisterAddressTransaction.h"
#include "gradido_blockchain/model/protobufWrapper/RegisterAddress.h"
#include "gradido_blockchain/model/CrossGroupTransactionBuilder.h"
#include "gradido_blockchain/model/TransactionFactory.h"


/*
	* addressType: human | project | subaccount
	* nameHash: optional, for finding on blockchain by name without need for asking community server
	* subaccountPubkey: optional, only set for address type SUBACCOUNT
	*
	{
		"created":"2021-01-10 10:00:00",
		"userName":"<apollo user identificator>",
		"addressType":"human",
		"nameHash":"",
		"subaccountPubkey":"",
		"currentGroupAlias": "gdd1",
		"newGroupAlias":"gdd2"
	}
	*/

using namespace rapidjson;

Document JsonRegisterAddressTransaction::handle(const rapidjson::Document& params)
{
	auto mm = MemoryManager::getInstance();

	auto paramError = readSharedParameter(params);
	if (paramError.IsObject()) { return paramError; }

	std::string userName, addressTypeString;
	std::string currentGroupAlias, newGroupAlias;

	getStringParameter(params, "userName", userName);
	paramError = getStringParameter(params, "addressType", addressTypeString);
	if (paramError.IsObject()) { return paramError; }
	auto addressType = model::gradido::RegisterAddress::getAddressTypeFromString(addressTypeString);
	if (addressType == proto::gradido::RegisterAddress_AddressType_SUBACCOUNT) {
		return stateError("subaccount address type currently not supported from this function");
	}

	getStringParameter(params, "currentGroupAlias", currentGroupAlias);
	getStringParameter(params, "newGroupAlias", newGroupAlias);

	MemoryBin* userRootPubkey = mm->getMemory(KeyPairEd25519::getPublicKeySize());
	memcpy(*userRootPubkey, mSession->getKeyPair()->getPublicKey(), KeyPairEd25519::getPublicKeySize());

	try {
		std::string lastIotaMessageId;
		auto baseTransaction = TransactionFactory::createRegisterAddress(userRootPubkey, addressType);

		if (currentGroupAlias.size() && newGroupAlias.size() && currentGroupAlias != newGroupAlias) {
			CrossGroupTransactionBuilder builder(std::move(baseTransaction));

			lastIotaMessageId = signAndSendTransaction(std::move(builder.createOutboundTransaction(newGroupAlias)), currentGroupAlias);			
			auto inbound = builder.createInboundTransaction(currentGroupAlias);
			auto iotaMessageIdBin = DataTypeConverter::hexToBin(lastIotaMessageId);
			inbound->setParentMessageId(iotaMessageIdBin);
			mm->releaseMemory(iotaMessageIdBin);

			lastIotaMessageId = signAndSendTransaction(std::move(inbound), newGroupAlias);
		}
		else {
			lastIotaMessageId = signAndSendTransaction(std::move(baseTransaction), mSession->getGroupAlias());
		}
		
		auto response = stateSuccess();
		auto alloc = response.GetAllocator();
		response.AddMember("iotaMessageId", Value(lastIotaMessageId.data(), lastIotaMessageId.size(), alloc), alloc);
		return response;
	}
	catch (...) {
		if (userRootPubkey) { mm->releaseMemory(userRootPubkey); }
		throw;
	}
	
}