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

		auto baseTransaction = TransactionFactory::createRegisterAddress(userRootPubkey, addressType);

		if (currentGroupAlias.size() && newGroupAlias.size()) {
			CrossGroupTransactionBuilder builder(std::move(baseTransaction));

			signAndSendTransaction(
				std::move(builder.createOutboundTransaction(newGroupAlias)),
				std::move(builder.createInboundTransaction(currentGroupAlias))
			);
		}
		else {
			signAndSendTransaction(std::move(baseTransaction));
		}
		// TODO
		return stateSuccess();
	}
	catch (...) {
		if (userRootPubkey) { mm->releaseMemory(userRootPubkey); }
		throw;
	}
	
}