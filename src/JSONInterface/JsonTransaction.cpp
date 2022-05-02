#include "JsonTransaction.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/Decay.h"
#include "gradido_blockchain/http/RequestExceptions.h"
#include "gradido_blockchain/http/IotaRequestExceptions.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"
#include "gradido_blockchain/crypto/KeyPairEd25519.h"

#include "SessionManager.h"
#include "ServerConfig.h"
#include "GradidoNodeRPC.h"
#include "model/PendingTransactions.h"
#include "model/table/Group.h"

#include "Poco/DateTimeParser.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Timezone.h"

#include "rapidjson/writer.h"


using namespace rapidjson;

Document JsonTransaction::readSharedParameter(const Document& params)
{
	getStringParameter(params, "memo", mMemo);

	std::string created_string, apollo_decay_start_string;
	auto paramError = getStringParameter(params, "created", created_string);
	getStringParameter(params, "apolloDecayStart", apollo_decay_start_string);
	if (paramError.IsObject()) { return paramError; }

	int timezoneDifferential = Poco::Timezone::tzd();
	try {
		mCreated = Poco::DateTimeParser::parse(created_string, timezoneDifferential);
	}
	catch (Poco::Exception& ex) {
		return stateError("cannot parse created", ex.what());
	}
	try {
		if (apollo_decay_start_string.size()) {
			mApolloDecayStart = Poco::DateTimeParser::parse(apollo_decay_start_string, timezoneDifferential);
		}
	}
	catch (Poco::Exception& ex) {
		return stateError("cannot parse apollo decay start", ex.what());
	}
	mApolloTransactionId = 0;
	getUInt64Parameter(params, "apolloTransactionId", mApolloTransactionId);
	getStringParameter(params, "apolloCreatedDecay", mApolloCreatedDecay);
	try {
		mSession = SessionManager::getInstance()->getSession(getJwtToken(), mClientIp.toString());
	}
	catch (JwtTokenException& ex) {
		Poco::Logger::get("errorLog").warning("jwt token exception: %s", ex.getFullString());
		return stateError("invalid jwt token");
	}
	catch (LoginException& ex) {
		return stateError(ex.what());
	}
	catch (SessionException& ex) {
		Poco::Logger::get("errorLog").warning("jwt token exception: %s", ex.getFullString());
		return stateError(ex.what());
	}
	return Document();
}

std::string JsonTransaction::signAndSendTransaction(std::unique_ptr<model::gradido::GradidoTransaction> transaction, const std::string& groupAlias)
{
	transaction->setMemo(mMemo).setCreated(mCreated).updateBodyBytes();
	auto transactionBody = transaction->getTransactionBody();
	
	if (!mSession->signTransaction(transaction.get())) {
		throw Ed25519SignException("cannot sign transaction", mSession->getPublicKey(), *transactionBody->getBodyBytes().get());
	}
	try {
		transaction->validate(model::gradido::TRANSACTION_VALIDATION_SINGLE);
	}
	catch (model::gradido::TransactionValidationInvalidSignatureException& ex) {
		printf("invalid signature exception: %s\n", ex.getFullString().data());
		throw;
	}
	
	std::string _groupAlias = groupAlias;
	
	if (!model::gradido::TransactionBase::isValidGroupAlias(_groupAlias)) {
		throw model::gradido::TransactionValidationInvalidInputException("invalid group alias", "groupAlias", "string, [a-z0-9-]{3,120}");
	}
	if (mApolloCreatedDecay.size()) {
		// throw an exception if apollo decay deviate to much from Gradido Node
		validateApolloDecay(transaction.get());
	}
		
	// send transaction to iota
	auto raw_message = transaction->getSerialized();

	// finale to hex for iota
	auto hex_message = DataTypeConverter::binToHex(std::move(raw_message));

	std::string index = "GRADIDO." + groupAlias;
	auto iotaMessageId = ServerConfig::g_IotaRequestHandler->sendMessage(DataTypeConverter::binToHex(index), *hex_message);
	auto pt = model::PendingTransactions::getInstance();
	pt->pushNewTransaction(std::move(model::PendingTransactions::PendingTransaction(
		iotaMessageId,
		transaction->getTransactionBody()->getTransactionType(),
		mApolloCreatedDecay,
		mApolloDecayStart,
		mApolloTransactionId
	)));
	return std::move(iotaMessageId);
}

bool JsonTransaction::validateApolloDecay(const model::gradido::GradidoTransaction* gradidoTransaction)
{	
	auto transactionBody = gradidoTransaction->getTransactionBody();
	std::string pubkeyHex;
	if (transactionBody->isCreation()) {
		pubkeyHex = DataTypeConverter::binToHex(
			transactionBody->getCreationTransaction()->getRecipientPublicKeyString()
		);
	}
	else if (transactionBody->isTransfer() || transactionBody->isDeferredTransfer()) {
		pubkeyHex = DataTypeConverter::binToHex(
			transactionBody->getTransferTransaction()->getSenderPublicKeyString()
		);
	}
	else {
		return true;
	}
	auto startBalanceString = gradidoNodeRPC::getAddressBalance(
		pubkeyHex, 
		Poco::DateTimeFormatter::format(mApolloDecayStart, Poco::DateTimeFormat::ISO8601_FRAC_FORMAT),
		mSession->getGroupAlias()
	);
	auto balance = MathMemory::create();
	auto startBalance = MathMemory::create();
	if (mpfr_set_str(startBalance->getData(), startBalanceString.data(), 10, gDefaultRound)) {
		std::string error = "invalid balance string " + startBalanceString;
		throw gradidoNodeRPC::GradidoNodeRPCException(error.data());
	}
	// copy start balance value also into balance variable
	mpfr_set(balance->getData(), startBalance->getData(), gDefaultRound);
	auto decayTemp = MathMemory::create();
	calculateDecayFactorForDuration(
		decayTemp->getData(),
		gDecayFactorGregorianCalender,
		(mCreated - mApolloDecayStart).totalSeconds()
	);
	// balance contain the end balance after function call
	calculateDecayFast(decayTemp->getData(), balance->getData());
	// calculate decay between start and end balance
	// decayTemp contain difference between end balance and start balance 
	mpfr_sub(decayTemp->getData(), startBalance->getData(), balance->getData(), gDefaultRound);
	// clear memory for start balance
	startBalance.reset();
	// compare with apollo value
	if (mpfr_set_str(balance->getData(), mApolloCreatedDecay.data(), 10, gDefaultRound)) {
		throw model::gradido::TransactionValidationInvalidInputException(
			"cannot parse to number",
			"apolloCreatedDecay",
			"number as string"
		);
	}
	// calculate difference between decay values
	auto diff = MathMemory::create();
	mpfr_sub(diff->getData(), balance->getData(), decayTemp->getData(), gDefaultRound);
	mpfr_abs(diff->getData(), diff->getData(), gDefaultRound);
	if (mpfr_cmp_d(diff->getData(), 0.00001) >= 0) {
		std::string decayString;
		model::gradido::TransactionBase::amountToString(&decayString, balance->getData());
		throw ApolloDecayException(
			"apollo decay deviates more than 0.00001 from Gradido Node decay",
			startBalanceString,
			decayString
		);
		/*throw model::gradido::TransactionValidationInvalidInputException(
			"divergence is more than 0.00001",
			"apolloCreatedDecay",
			"number as string"
		);*/
	}
	return true;

}

rapidjson::Document JsonTransaction::handleSignAndSendTransactionExceptions()
{
	try {
		throw; // assume it was called from catch clause
	}
	catch (model::gradido::TransactionValidationInvalidSignatureException& ex) {
		Poco::Logger::get("errorLog").error("invalid signature exception: %s", ex.getFullString());
		return stateError("Internal Server Error");
	}
	catch (model::gradido::TransactionValidationInvalidInputException& ex) {
		return stateError("transaction validation failed", ex);
	}
	catch (IotaRequestException& ex) {
		Poco::Logger::get("errorLog").error("error by calling iota: %s", ex.getFullString());
		return stateError("error by calling iota", ex);
	}	
	catch (RapidjsonParseErrorException& ex) {
		Poco::Logger::get("errorLog").error("calling iota return invalid json: %s", ex.getFullString());
		return stateError("error by calling iota", ex);
	}
	catch (Ed25519SignException& ex) {
		Poco::Logger::get("errorLog").error("error by signing a transaction: %s", ex.getFullString());
		return stateError("Internal Server Error");
	}
	catch (gradidoNodeRPC::GradidoNodeRPCException& ex) {
		Poco::Logger::get("errorLog").error("error by requesting Gradido Node: %s", ex.getFullString());
		return stateError("error by requesting Gradido Node");
	}
	catch (ApolloDecayException& ex) {
		return stateError("error with apollo decay", ex);
	}

	return Document();
}


std::string JsonTransaction::encryptMemo(const std::string& memo, const unsigned char* ed25519Pubkey, const MemoryBin* ed25519Privkey)
{
	auto mm = MemoryManager::getInstance();
	// encrypt memo
	KeyPairEd25519 pubkey(ed25519Pubkey);
	MemoryBin* privKeyx25519 = mm->getMemory(AuthenticatedEncryption::getPrivateKeySize());
	crypto_sign_ed25519_sk_to_curve25519(privKeyx25519->data(), ed25519Privkey->data());
	AuthenticatedEncryption senderKey(privKeyx25519);
	AuthenticatedEncryption recipientKey(&pubkey);
	auto encryptedMemoBin = senderKey.encrypt(memo, &recipientKey);
	auto resultMemo = DataTypeConverter::binToBase64(encryptedMemoBin);
	mm->releaseMemory(encryptedMemoBin);
	return std::move(resultMemo);
}

std::string JsonTransaction::decryptMemo(const std::string& base64EncodedEncryptedMemo, const unsigned char* ed25519Pubkey, const MemoryBin* ed25519Privkey)
{
	auto mm = MemoryManager::getInstance();
	auto encryptedMemoBin = DataTypeConverter::base64ToBin(base64EncodedEncryptedMemo);
	KeyPairEd25519 pubkey(ed25519Pubkey);
	MemoryBin* privKeyx25519 = mm->getMemory(AuthenticatedEncryption::getPrivateKeySize());
	crypto_sign_ed25519_sk_to_curve25519(privKeyx25519->data(), ed25519Privkey->data());
	AuthenticatedEncryption senderKey(&pubkey);
	AuthenticatedEncryption recipientKey(privKeyx25519);
	auto clearMemo = recipientKey.decrypt(encryptedMemoBin, &senderKey);
	if (!clearMemo) {
		return "";
	}
	auto clearMemoString = clearMemo->copyAsString();
	std::string resultMemo((const char*)clearMemo->data(), clearMemo->size());
	mm->releaseMemory(clearMemo);
	return std::move(resultMemo);
}

// *******************  Apollo Decay Exception ***********************
ApolloDecayException::ApolloDecayException(const char* what, std::string startBalance, std::string decay) noexcept
	: GradidoBlockchainException(what), mStartBalance(startBalance), mDecay(decay)
{

}

std::string ApolloDecayException::getFullString() const
{
	std::string result;
	result = what();
	result += ", startBalance: " + mStartBalance;
	result += ", decay: " + mDecay;
	return result;
}


Value ApolloDecayException::getDetails(Document::AllocatorType& alloc) const
{
	Value detailsObjs(kObjectType);
	detailsObjs.AddMember("what", Value(what(), alloc), alloc);
	detailsObjs.AddMember("startBalance", Value(mStartBalance.data(), alloc), alloc);
	detailsObjs.AddMember("decay", Value(mDecay.data(), alloc), alloc);

	return std::move(detailsObjs);
}