#include "JsonCreationTransaction.h"
#include "GradidoNodeRPC.h"

#include "Poco/Timezone.h"
#include "Poco/DateTimeParser.h"

#include "model/table/User.h"

#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"
#include "gradido_blockchain/lib/Decay.h"
#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"

using namespace rapidjson;


	/*
	{
		"created":"2021-01-10 10:00:00",
		"memo": "AGE September 2021",
		"recipientName":"<apollo user identificator>",
		"amount": "100",
		"targetDate": "2021-09-01 01:00:00",
	}
	and jwtoken with user informations
	*/

Document JsonCreationTransaction::handle(const rapidjson::Document& params)
{
	auto mm = MemoryManager::getInstance();

	auto paramError = readSharedParameter(params);
	if (paramError.IsObject()) { return paramError; }

	std::string recipientName, amountString, targetDateString;
	uint64_t apolloTransactionId = 0;
	paramError = getStringParameter(params, "recipientName", recipientName);
	if (paramError.IsObject()) { return paramError;}

	paramError = getStringParameter(params, "amount", amountString);
	if (paramError.IsObject()) { return paramError;}

	std::string coinGroupId;
	getStringParameter(params, "coinGroupId", coinGroupId);

	Poco::DateTime targetDate;
	paramError = getStringParameter(params, "targetDate", targetDateString);
	if (paramError.IsObject()) { return paramError; }
	int timezoneDifferential = Poco::Timezone::tzd();
	try {
		targetDate = Poco::DateTimeParser::parse(targetDateString, timezoneDifferential);
		targetDate.makeLocal(Poco::Timezone::tzd());
	}
	catch (Poco::Exception& ex) {
		return stateError("cannot parse targetDate", ex.what());
	}

	auto recipientUser = model::table::User::load(recipientName, mSession->getGroupId());
	if (!recipientUser) {
		return stateError("unknown recipient user");
	}
	auto publicKeyBin = mm->getMemory(32);
	publicKeyBin->copyFromProtoBytes(recipientUser->getPublicKey());
	auto pubkeyHex = DataTypeConverter::binToHex(publicKeyBin);

	// encrypt memo
	mMemo = encryptMemo(mMemo, publicKeyBin->data(), mSession->getKeyPair()->getPrivateKey());

	try {
		if (!mArchiveTransaction) 
		{
			auto addressType = gradidoNodeRPC::getAddressType(pubkeyHex, mSession->getGroupAlias());
			if (addressType != model::gradido::RegisterAddress::getAddressStringFromType(proto::gradido::RegisterAddress_AddressType_HUMAN)) {
				Poco::Logger::get("errorLog").warning("address type: %s isn't allowed for creation", addressType);
				if (addressType == model::gradido::RegisterAddress::getAddressStringFromType(proto::gradido::RegisterAddress_AddressType_NONE)) {
					return stateError("address isn't registered on blockchain");
				}
				return stateError("address has the wrong type for creation");
			}
		
			auto sumString = gradidoNodeRPC::getCreationSumForMonth(
				pubkeyHex, targetDate.month(), targetDate.year(),
				Poco::DateTimeFormatter::format(mCreated, Poco::DateTimeFormat::ISO8601_FRAC_FORMAT),
				mSession->getGroupAlias()
			);

			auto sum = MathMemory::create();
			if (mpfr_set_str(sum->getData(), sumString.data(), 10, gDefaultRound)) {
				std::string error = "cannot parse sum from Gradido Node: %s" + sumString;
				throw gradidoNodeRPC::GradidoNodeRPCException(error.data());
			}
			auto amount = MathMemory::create();
			if (mpfr_set_str(amount->getData(), amountString.data(), 10, gDefaultRound)) {
				throw model::gradido::TransactionValidationInvalidInputException(
					"amount cannot be parsed to a number",
					"amount", "amount as string"
				);
			}
			mpfr_add(sum->getData(), sum->getData(), amount->getData(), gDefaultRound);
			// if sum > 1000
			if (mpfr_cmp_d(sum->getData(), 1000.0) > 0) {
				throw model::gradido::TransactionValidationInvalidInputException(
					"creation more than 1.000 GDD per month not allowed",
					"amount"
				);
			}
		}

		auto creation = TransactionFactory::createTransactionCreation(publicKeyBin, amountString, targetDate);
		mm->releaseMemory(publicKeyBin);
		publicKeyBin = nullptr;

		auto iotaMessageId = signAndSendTransaction(std::move(creation), mSession->getGroupAlias());
		auto response = stateSuccess();
		auto alloc = response.GetAllocator();
		response.AddMember("iotaMessageId", Value(iotaMessageId.data(), iotaMessageId.size(), alloc), alloc);
		return response;
	}
	catch (...) {
		if(publicKeyBin) mm->releaseMemory(publicKeyBin);
		return handleSignAndSendTransactionExceptions();
	}

}