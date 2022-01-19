#include "JsonPackTransaction.h"

#include "../lib/DataTypeConverter.h"

#include "../model/gradido/Transaction.h"

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
	else if (transactionType == "groupMemberUpdate") {
		return groupMemberUpdate(params);
	}

	return stateError("transaction_type unknown");

}

Document JsonPackTransaction::transfer(const Document& params)
{
	/*
	* 
	{
		"senderPubkey":"131c7f68dd94b2be4c913400ff7ff4cdc03ac2bda99c2d29edcacb3b065c67e6",
		"recipientPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"amount": 1000000,
		"senderGroupAlias": "gdd1",
		"recipientGroupAlias":"gdd2"
	}
	*/
	std::string senderPubkey, recipientPubkey, senderGroupAlias, recipientGroupAlias;
	Poco::Int64 amount = 0;

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
	if (senderGroupAlias.size() && recipientGroupAlias.size() && senderGroupAlias != recipientGroupAlias) {
		auto transactionArray = model::gradido::Transaction::createTransferCrossGroup(
			senderPubkeyBin,
			recipientPubkeyBin,
			senderGroupAlias, 
			recipientGroupAlias,
			amount,
			mMemo
		);
		transactions.push_back({ transactionArray[0], senderGroupAlias }); // outbound
		transactions.push_back({ transactionArray[1], recipientGroupAlias }); // inbound
	}
	else {
		auto transaction = model::gradido::Transaction::createTransferLocal(senderPubkeyBin, recipientPubkeyBin, amount, mMemo);
		transactions.push_back({ transaction, "" });
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

	std::string targetDateString;
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
	transactions.push_back({ model::gradido::Transaction::createCreation(recipientPubkeyBin, amount, targetDate, mMemo), "" });
	mm->releaseMemory(recipientPubkeyBin);

	return resultBase64Transactions(transactions);

}
Document JsonPackTransaction::groupMemberUpdate(const Document& params)
{
	/*
	*
	{
		"transactionType": "groupMemberUpdate",
		"created":"2021-01-10 10:00:00",
		"userRootPubkey":"eff7a4a440eb10fa6d5ae5ee47d63240c55ea3e1972e9815c09411e25ab09fdd",
		"currentGroupAlias": "gdd1",
		"newGroupAlias":"gdd2"
	}
	*/
	std::string userRootPubkey;
	std::string currentGroupAlias, newGroupAlias;

	auto paramError = getStringParameter(params, "userRootPubkey", userRootPubkey);
	if (paramError.IsObject()) { return paramError; }

	getStringParameter(params, "currentGroupAlias", currentGroupAlias);
	getStringParameter(params, "newGroupAlias", newGroupAlias);

	auto mm = MemoryManager::getInstance();

	auto userRootPubkeyBin = DataTypeConverter::hexToBin(userRootPubkey);

	std::vector<TransactionGroupAlias> transactions;
	if (currentGroupAlias.size() && newGroupAlias.size()) {
		auto transactionArray = model::gradido::Transaction::createGroupMemberUpdateMove(userRootPubkeyBin, currentGroupAlias, newGroupAlias);
		transactions.push_back({ transactionArray[0], currentGroupAlias }); // outbound
		transactions.push_back({ transactionArray[1], newGroupAlias }); // inbound
	}
	else {
		transactions.push_back({ model::gradido::Transaction::createGroupMemberUpdateAdd(userRootPubkeyBin), "" });
	}
	mm->releaseMemory(userRootPubkeyBin);

	return resultBase64Transactions(transactions);

}


Document JsonPackTransaction::resultBase64Transactions(std::vector<TransactionGroupAlias> transactions)
{	
	Value transactionsJsonArray(kArrayType);
	auto result = stateSuccess();
	auto alloc = result.GetAllocator();

	for (auto it = transactions.begin(); it != transactions.end(); it++) {
		auto transactionBody = it->first->getTransactionBody();
		transactionBody->setCreated(mCreated);
		auto result = transactionBody->getTransactionBase()->validate();
		if (result != model::gradido::TRANSACTION_VALID_OK) {
			return stateError("invalid transaction", transactionBody->getTransactionBase());
		}
		Value entry(kObjectType);
		
		if (it->second.size()) {
			entry.AddMember("groupAlias", Value(it->second.data(), alloc), alloc);
		}
		try {
			std::string base64 = DataTypeConverter::binToBase64(transactionBody->getBodyBytes());
			if (base64 != "<uninitalized>") {
				entry.AddMember("bodyBytesBase64", Value(base64.data(), alloc), alloc);
				printf("body bytes hex: %s\n", DataTypeConverter::binToHex(transactionBody->getBodyBytes()).data());
			}
			else {
				return stateError("invalid body bytes");
			}
			transactionsJsonArray.PushBack(entry, alloc);
		}
		catch (Poco::Exception& ex) {
			return stateError("exception in serializing", ex.what());
		}
	}
	
	result.AddMember("transactions", transactionsJsonArray, alloc);
	return result;
}