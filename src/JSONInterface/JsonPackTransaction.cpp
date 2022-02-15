#include "JsonPackTransaction.h"

#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/http/RequestExceptions.h"

using namespace rapidjson;

Document JsonPackTransaction::handle(const Document& params)
{	
	std::string transactionType;
	auto paramError = getStringParameter(params, "transactionType", transactionType);
	if (paramError.IsObject()) { return paramError; }

	getStringParameter(params, "memo", mMemo);	

	std::string created_string;
	paramError = getStringParameter(params, "created", created_string);
	if (paramError.IsObject()) { return paramError; }
	int timezoneDifferential = 0;
	try {
		mCreated = Poco::DateTimeParser::parse(created_string, timezoneDifferential);
	}
	catch (Poco::Exception& ex) {
		return stateError("cannot parse created", ex.what());
	}
	
	if (transactionType == "transfer") {
		return transfer(params);
	}
	else if (transactionType == "creation") {
		return creation(params);
	}
	else if (transactionType == "registerAddress") {
		return registerAddress(params);
	}

	return stateError("transaction_type unknown");

}

Document JsonPackTransaction::transfer(const Document& params)
{
	/*
	* 
	{
		"created":"2021-01-10 10:00:00",
		"senderPubkey":"131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6",
		"recipientPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"amount": 1000000,
		"coinColor": "ffffffff",
		"senderGroupAlias": "gdd1",
		"recipientGroupAlias":"gdd2"
	}
	*/
	std::string senderPubkey, recipientPubkey, senderGroupAlias, recipientGroupAlias;
	Poco::Int64 amount = 0;
	Poco::UInt32 coinColor = 0;

	auto paramError = getStringParameter(params, "senderPubkey", senderPubkey);
	if (paramError.IsObject()) { return paramError; }

	paramError = getStringParameter(params, "recipientPubkey", recipientPubkey);
	if (paramError.IsObject()) { return paramError; }

	auto param_error = getInt64Parameter(params, "amount", amount);
	if (param_error.IsObject()) { return param_error; }


	getStringParameter(params, "senderGroupAlias", senderGroupAlias);
	getStringParameter(params, "recipientGroupAlias", recipientGroupAlias);

	auto mm = MemoryManager::getInstance();
	auto senderPubkeyBin = DataTypeConverter::hexToBin(senderPubkey);
	auto recipientPubkeyBin = DataTypeConverter::hexToBin(recipientPubkey);

	std::vector<TransactionGroupAlias> transactions;
	auto baseTransaction = TransactionFactory::createTransactionTransfer(
		senderPubkeyBin,
		amount,
		readCoinColor(params),
		recipientPubkeyBin
	);

	if (senderGroupAlias.size() && recipientGroupAlias.size()) {
		CrossGroupTransactionBuilder builder(std::move(baseTransaction));

		transactions.push_back({ builder.createOutboundTransaction(recipientGroupAlias).release(), senderGroupAlias }); // outbound
		transactions.push_back({ builder.createInboundTransaction(senderGroupAlias).release(), recipientGroupAlias }); // inbound
	}
	else {
		transactions.push_back({ baseTransaction.release(), "" });
	}

	mm->releaseMemory(senderPubkeyBin);
	mm->releaseMemory(recipientPubkeyBin);
	
	return resultBase64Transactions(transactions);
}

Document JsonPackTransaction::creation(const Document& params)
{
	/*
	*
	{
		"transactionType": "creation",
		"created":"2021-01-10 10:00:00",
		"memo": "AGE September 2021",
		"recipientPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"amount": 10000000,
		"coinColor": "ffffffff",
		"targetDate": "2021-09-01 01:00:00",
	}
	*/
	std::string recipientPubkey;
	Poco::Int64 amount = 0;
	Poco::DateTime targetDate;

	auto paramError = getStringParameter(params, "recipientPubkey", recipientPubkey);
	if (paramError.IsObject()) { return paramError; }

	paramError = getInt64Parameter(params, "amount", amount);
	if (paramError.IsObject()) { return paramError; }

	std::string targetDateString, createdString;
	paramError = getStringParameter(params, "targetDate", targetDateString);
	if (paramError.IsObject()) { return paramError; }
	int timezoneDifferential = 0;
	try {
		targetDate = Poco::DateTimeParser::parse(targetDateString, timezoneDifferential);
	}
	catch (Poco::Exception& ex) {
		return stateError("cannot parse targetDate", ex.what());
	}	

	auto mm = MemoryManager::getInstance();
	auto recipientPubkeyBin = DataTypeConverter::hexToBin(recipientPubkey);

	std::vector<TransactionGroupAlias> transactions;
	try {
		auto creation = TransactionFactory::createTransactionCreation(recipientPubkeyBin, amount, readCoinColor(params), targetDate);

		transactions.push_back({ creation.release(), "" });
	}
	catch (...) {
		mm->releaseMemory(recipientPubkeyBin);
		throw;
	}

	return resultBase64Transactions(transactions);

}
Document JsonPackTransaction::registerAddress(const Document& params)
{
	
	/*
	* addressType: human | project | subaccount
	* nameHash: optional, for finding on blockchain by name without need for asking community server
	* subaccountPubkey: optional, only set for address type SUBACCOUNT
	*
	{
		"transactionType": "registerAddress",
		"created":"2021-01-10 10:00:00",
		"userRootPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"addressType":"human",			
		"nameHash":"",
		"subaccountPubkey":"",
		"currentGroupAlias": "gdd1",
		"newGroupAlias":"gdd2"
	}
	*/
	std::string userRootPubkeyHex, addressTypeString, nameHashHex, subaccountPubkeyHex;
	std::string currentGroupAlias, newGroupAlias;

	getStringParameter(params, "userRootPubkey", userRootPubkeyHex);
	auto paramError = getStringParameter(params, "addressType", addressTypeString);
	if (paramError.IsObject()) { return paramError; }
	auto addressType = model::gradido::RegisterAddress::getAddressTypeFromString(addressTypeString);
	getStringParameter(params, "nameHash", nameHashHex);
	getStringParameter(params, "subaccountPubkey", subaccountPubkeyHex);
		
	if (!userRootPubkeyHex.size() && !subaccountPubkeyHex.size()) {
		return stateError("userRootPubkey or subaccountPubkey must be given");
	}

	getStringParameter(params, "currentGroupAlias", currentGroupAlias);
	getStringParameter(params, "newGroupAlias", newGroupAlias);

	auto mm = MemoryManager::getInstance();
	MemoryBin* userRootPubkey = nullptr;
	MemoryBin* nameHash = nullptr;
	MemoryBin* subaccountPubkey = nullptr;

	std::vector<TransactionGroupAlias> transactions;
	try {

		if (userRootPubkeyHex.size()) { userRootPubkey = DataTypeConverter::hexToBin(userRootPubkeyHex); }
		if (nameHashHex.size()) { nameHash = DataTypeConverter::hexToBin(nameHashHex); }
		if (subaccountPubkeyHex.size()) { subaccountPubkey = DataTypeConverter::hexToBin(subaccountPubkeyHex); }

		auto baseTransaction = TransactionFactory::createRegisterAddress(userRootPubkey, addressType, nameHash, subaccountPubkey);		

		if (currentGroupAlias.size() && newGroupAlias.size()) {
			CrossGroupTransactionBuilder builder(std::move(baseTransaction));

			transactions.push_back({ builder.createOutboundTransaction(newGroupAlias).release(), currentGroupAlias }); // outbound
			transactions.push_back({ builder.createInboundTransaction(currentGroupAlias).release(), newGroupAlias }); // inbound
		}
		else {
			transactions.push_back({ baseTransaction.release(), "" });
		}
	}
	catch (...) {
		if (userRootPubkey) { mm->releaseMemory(userRootPubkey); }
		if (nameHash) { mm->releaseMemory(nameHash); }
		if (subaccountPubkey) { mm->releaseMemory(subaccountPubkey); }
		throw;
	}

	
	return resultBase64Transactions(transactions);

}


Document JsonPackTransaction::resultBase64Transactions(std::vector<TransactionGroupAlias> transactions)
{	
	Value transactionsJsonArray(kArrayType);
	auto result = stateSuccess();
	auto alloc = result.GetAllocator();
	for (auto it = transactions.begin(); it != transactions.end(); it++) {
		it->first->setMemo(mMemo).setCreated(mCreated);

		auto transactionBody = it->first->getTransactionBody();

		if(!transactionBody->getTransactionBase()->validate()) {
			return stateError("invalid transaction", it->first->toJson());
		}
		Value entry(kObjectType);
		
		if (it->second.size()) {
			entry.AddMember("groupAlias", Value(it->second.data(), alloc), alloc);
		}
		try {
			
			auto base64 = DataTypeConverter::binToBase64(transactionBody->getBodyBytes());
			entry.AddMember("bodyBytesBase64", Value(base64->data(), base64->size(), alloc), alloc);
			
			transactionsJsonArray.PushBack(entry, alloc);
		}
		catch (Poco::Exception& ex) {
			return stateError("exception in serializing", ex.what());
		}
	}
	
	result.AddMember("transactions", transactionsJsonArray, alloc);
	return result;
}

uint32_t JsonPackTransaction::readCoinColor(const rapidjson::Document& params)
{
	auto coinColor = params.FindMember("coinColor");
	if (coinColor == params.MemberEnd()) {
		return 0;
	}
	if (coinColor->value.IsString()) {
		std::string coinColorString = coinColor->value.GetString();
		auto coinColorBin = DataTypeConverter::hexToBin(coinColorString);
		if (!coinColorBin || coinColorBin->size() != sizeof(uint32_t)) {
			throw HandleRequestException("coinColor isn't a valid hex string");
		}
		uint32_t result;
		memcpy(&result, *coinColorBin, sizeof(uint32_t));
		return result;		
	}
	else if (coinColor->value.IsUint()) {
		return coinColor->value.GetUint();
	} 
	throw HandleRequestException("coinColor has unknown type");
}