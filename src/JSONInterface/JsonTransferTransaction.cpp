#include "JsonTransferTransaction.h"

#include "model/table/User.h"
#include "model/table/Group.h"
#include "../GradidoNodeRPC.h"

#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/CrossGroupTransactionBuilder.h"
#include "gradido_blockchain/http/JsonRPCRequest.h"
#include "gradido_blockchain/lib/Decay.h"

using namespace rapidjson;

/*
	{
		"created":"2021-01-10 10:00:00",
		"recipientName":"<name>",
		"amount": "100",
		"coinColor": "ffffffff",
		"recipientGroupAlias":"gdd2"
	}
*/

Document JsonTransferTransaction::handle(const rapidjson::Document& params)
{
	auto mm = MemoryManager::getInstance();

	auto paramError = readSharedParameter(params);
	if (paramError.IsObject()) { return paramError; }

	std::string recipientName, amountString, recipientGroupAlias;
	paramError = getStringParameter(params, "recipientName", recipientName);
	if (paramError.IsObject()) { return paramError; }

	getStringParameter(params, "recipientGroupAlias", recipientGroupAlias);
	int targetGroupId = mSession->getGroupId();
	if (recipientGroupAlias.size()) {
		try {
			auto group = model::table::Group::load(recipientGroupAlias);
			targetGroupId = group->getId();
		}
		catch (model::table::RowNotFoundException& ex) {

		}
	}

	paramError = getStringParameter(params, "amount", amountString);
	if (paramError.IsObject()) { return paramError; }

	auto coinColor = readCoinColor(params);

	auto recipientUser = model::table::User::load(recipientName, targetGroupId);
	if (!recipientUser) {
		return stateError("unknown recipient user");
	}
	auto senderPublicKey = mm->getMemory(KeyPairEd25519::getPublicKeySize());
	memcpy(*senderPublicKey, mSession->getPublicKey(), KeyPairEd25519::getPublicKeySize());

	auto recipientPublicKey = mm->getMemory(KeyPairEd25519::getPublicKeySize());
	recipientPublicKey->copyFromProtoBytes(recipientUser->getPublicKey());

	// check if balance is enough on gradido node
	try {
		std::string created;
		getStringParameter(params, "created", created);
		auto balanceString = gradidoNodeRPC::getAddressBalance(mSession->getPublicKeyHex(), created, mSession->getGroupAlias(), coinColor);
		auto balance = MathMemory::create();
		if (mpfr_set_str(balance->getData(), balanceString.data(), 10, gDefaultRound)) {
			Poco::Logger::get("errorLog").critical("cannot parse balance string from gradido node: %s", balanceString);
			return stateError("error by requesting Gradido Node");
		}
		auto amount = MathMemory::create();
		if (mpfr_set_str(amount->getData(), amountString.data(), 10, gDefaultRound)) {
			return stateError("cannot parse amount to number");
		}
		if (mpfr_cmp(amount->getData(), balance->getData()) > 0) {
			// if op1 > op2
			return stateError("insufficient balance");
		}
	}
	catch (gradidoNodeRPC::GradidoNodeRPCException& ex) {
		return stateError("error by requesting Gradido Node");
	}

	try {
		std::string lastIotaMessageId;
		auto baseTransaction = TransactionFactory::createTransactionTransfer(senderPublicKey, amountString, coinColor, recipientPublicKey);
		mm->releaseMemory(senderPublicKey);
		senderPublicKey = nullptr;

		mm->releaseMemory(recipientPublicKey);
		recipientPublicKey = nullptr;

		if (recipientGroupAlias.size() && recipientGroupAlias != mSession->getGroupAlias()) {
			CrossGroupTransactionBuilder builder(std::move(baseTransaction));

			lastIotaMessageId = signAndSendTransaction(std::move(builder.createOutboundTransaction(recipientGroupAlias)), mSession->getGroupAlias());
			auto inbound = builder.createInboundTransaction(mSession->getGroupAlias());
			auto iotaMessageIdBin = DataTypeConverter::hexToBin(lastIotaMessageId);
			inbound->setParentMessageId(iotaMessageIdBin);
			mm->releaseMemory(iotaMessageIdBin);

			lastIotaMessageId = signAndSendTransaction(std::move(inbound), recipientGroupAlias);
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
		if (senderPublicKey) mm->releaseMemory(senderPublicKey);
		if (recipientPublicKey) mm->releaseMemory(recipientPublicKey);
		throw;
	}

}