#include "CompareApolloWithGradidoNode.h"
#include "ServerConfig.h"
#include "GradidoNodeRPC.h"
#include "ConnectionManager.h"

#include "gradido_blockchain/lib/Decay.h"
#include "gradido_blockchain/lib/Decimal.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

#include "Poco/Data/Statement.h"
#include "Poco/Logger.h"

using namespace Poco::Data::Keywords;

namespace task {
	CompareApolloWithGradidoNode::CompareApolloWithGradidoNode(uint64_t userId, const std::string& userPublicKeyHex, const std::string& groupAlias)
		: CPUTask(ServerConfig::g_WorkerThread), mUserId(userId), mUserPublicKeyHex(userPublicKeyHex), mGroupAlias(groupAlias), mIdentical(false)
	{
#ifdef _UNI_LIB_DEBUG
		setName(std::to_string(userId).data());
#endif
	}
	CompareApolloWithGradidoNode::~CompareApolloWithGradidoNode()
	{

	}

	int CompareApolloWithGradidoNode::run()
	{
		auto dbSession = ConnectionManager::getInstance()->getConnection();
		Poco::Data::Statement select(dbSession);
		auto mm = MemoryManager::getInstance();
		auto nodeBalance = MathMemory::create();
		auto dbBalance = MathMemory::create();
		auto decayForDuration = MathMemory::create();
		auto nodeDecay = MathMemory::create();
		auto dbDecay = MathMemory::create();
		auto nodeAmount = MathMemory::create();
		auto dbAmount = MathMemory::create();
		Poco::Logger& errorLog = Poco::Logger::get("errorLog");
		// step through transactions
		std::vector<Poco::Tuple<
			uint64_t, uint64_t, uint8_t, std::string,
			std::string, Poco::DateTime,
			std::string, Poco::DateTime
			>> dbTansactions;

		select << "select id, previous, type_id, CONVERT(amount, varchar(45)), "
			<< "CONVERT(balance, varchar(45)), balance_date, "
			<< "CONVERT(decay, varchar(45)), decay_start "
			<< "from transactions "
			<< "where user_id = ? ORDER BY balance_date ASC", use(mUserId), into(dbTansactions);
		auto rowCount = select.execute();

		auto nodeTransactions = gradidoNodeRPC::getTransactionsForUser(mUserPublicKeyHex, mGroupAlias, rowCount + 1);
		int64_t nodeTransactionCount = nodeTransactions.size();
		if (!nodeTransactionCount && !dbTansactions.size()) {
			logSuccess(dbTansactions.size());
			mIdentical = true;
			return 0;
		}
		if (nodeTransactionCount - 1 != dbTansactions.size()) {
			std::stringstream ss;

			mErrorString = "transaction count don't match, node: ";
			mErrorString += std::to_string(nodeTransactionCount - 1);
			mErrorString += ", db: " + std::to_string(dbTansactions.size());
			return 0;
		}
		Poco::DateTime now;
		Poco::DateTime lastBalanceDate(now);
		for (int i = 0; i < nodeTransactions.size(); i++) {
			auto serializedTransactionBin = DataTypeConverter::base64ToBinString(nodeTransactions[i]);
			model::gradido::GradidoBlock gradidoBlock(&serializedTransactionBin);
			auto body = gradidoBlock.getGradidoTransaction()->getTransactionBody();
			if (!i) {
				if (body->isRegisterAddress()) {
					continue;
				}
				mErrorString = "first node transaction isn't register address transaction: ";
				mErrorString += getNodeTransactionType(body);
				return 0;
			}
			auto dbTransaction = dbTansactions[i - 1];
			auto dbTransactionTypeId = dbTransaction.get<2>();

			if ((body->isCreation() && dbTransactionTypeId != 1) ||
				(body->isTransfer() && dbTransactionTypeId != 2 && dbTransactionTypeId != 3)) {
				mErrorString = "transaction (";
				mErrorString += std::to_string(i - 1);
				mErrorString += ") types dont' match, node: ";
				mErrorString += getNodeTransactionType(body);
				mErrorString += ", db: ";
				mErrorString += getDBTransactionType(dbTransactionTypeId);
				return 0;
			}
			mpfr_set_str(*dbAmount, dbTransaction.get<3>().data(), 10, gDefaultRound);
			mpfr_abs(*dbAmount, *dbAmount, gDefaultRound);
			mpfr_set_str(*dbBalance, dbTransaction.get<4>().data(), 10, gDefaultRound);
			mpfr_set_str(*dbDecay, dbTransaction.get<6>().data(), 10, gDefaultRound);

			auto userPublicKey = DataTypeConverter::hexToBinString(mUserPublicKeyHex)->substr(0, 32);
			Poco::DateTime localDate = Poco::Timestamp((Poco::UInt64)body->getCreatedSeconds() * Poco::Timestamp::resolution());
			if (body->isTransfer()) {
				auto transfer = body->getTransferTransaction();
				mpfr_set_str(*nodeAmount, transfer->getAmount().data(), 10, gDefaultRound);
				if (!isDecimalIdentical(*dbAmount, *nodeAmount, "amount")) {
					return 0;
				}
				if (!transfer->getSenderPublicKeyString().compare(userPublicKey) && dbTransactionTypeId != 2) {
					mErrorString = "node send transaction isn't send on apollo: ";
					mErrorString += getDBTransactionType(dbTransactionTypeId);
					return 0;
				}
			}
			else if (body->isCreation()) {
				auto creation = body->getCreationTransaction();
				mpfr_set_str(*nodeAmount, creation->getAmount().data(), 10, gDefaultRound);
				if (!isDecimalIdentical(*dbAmount, *nodeAmount, "amount")) {
					return 0;
				}
			}
			else {
				mErrorString = "node transaction type not yet supported: ";
				mErrorString += getNodeTransactionType(body);
				return 0;
			}
			if (lastBalanceDate != now && lastBalanceDate != localDate) {
				mpfr_set(nodeDecay->getData(), nodeBalance->getData(), gDefaultRound);
				calculateDecayFactorForDuration(*decayForDuration, gDecayFactorGregorianCalender, lastBalanceDate, localDate);
				calculateDecayFast(*decayForDuration, *nodeBalance);
				mpfr_sub(*nodeDecay, *nodeBalance, *nodeDecay, gDefaultRound);
			}
			if (!isDecimalIdentical(*nodeDecay, *dbDecay, "decay")) {
				auto decayStartDate = dbTransaction.get<7>();
				auto dbBalanceDate = dbTransaction.get<5>();
				auto nodeDiff = calculateDecayDurationSeconds(lastBalanceDate.timestamp(), localDate.timestamp());
				auto dbDiff = dbBalanceDate - decayStartDate;
				Poco::DateTime start = lastBalanceDate > DECAY_START_TIME ? lastBalanceDate : DECAY_START_TIME;
				mErrorString += "\ndates node: ";
				mErrorString += Poco::DateTimeFormatter::format(start, Poco::DateTimeFormat::SORTABLE_FORMAT);
				mErrorString += " - ";
				mErrorString += Poco::DateTimeFormatter::format(localDate, Poco::DateTimeFormat::SORTABLE_FORMAT);
				mErrorString += "\ndates db  : ";
				mErrorString += Poco::DateTimeFormatter::format(decayStartDate, Poco::DateTimeFormat::SORTABLE_FORMAT);
				mErrorString += " - ";
				mErrorString += Poco::DateTimeFormatter::format(dbBalanceDate, Poco::DateTimeFormat::SORTABLE_FORMAT);
				mErrorString += "\nseconds decay: \nnode: ";
				mErrorString += std::to_string(nodeDiff.totalSeconds()) + " seconds\ndb  : ";
				mErrorString += std::to_string(dbDiff.totalSeconds()) + " seconds";

				Decimal dbBalanceD(dbTransaction.get<4>().data());
				Decimal nodeBalanceD(*nodeBalance);
				mErrorString += "\nbalances, \nnode: " + nodeBalanceD.toString();
				mErrorString += "\ndb  : " + dbBalanceD.toString();

				return 0;
			}
			lastBalanceDate = localDate;
			if (dbTransactionTypeId == 2) {
				mpfr_sub(*nodeBalance, *nodeBalance, *nodeAmount, gDefaultRound);
			}
			else {
				mpfr_add(*nodeBalance, *nodeBalance, *nodeAmount, gDefaultRound);
			}
			if (mUserId == 11) {
				std::string nodeBalanceString;
				model::gradido::TransactionBase::amountToString(&nodeBalanceString, *nodeBalance);
				errorLog.information("user 11: node balance: %s", nodeBalanceString);
			}
			if (!isDecimalIdentical(*nodeBalance, *dbBalance, "balance")) {
				return 0;
			}
			// remove small error
			mpfr_set(nodeBalance->getData(), dbBalance->getData(), gDefaultRound);

			// Gradido Block contain final balance on creation and send transactions
			if (dbTransactionTypeId == 1 || dbTransactionTypeId == 2) {
				auto nodeFinalBalance = MathMemory::create();
				mpfr_set_str(*nodeFinalBalance, gradidoBlock.getFinalBalance().data(), 10, gDefaultRound);
				if (!isDecimalIdentical(*nodeFinalBalance, *dbBalance, "final balance")) {
					return 0;
				}
			}

		}

		// end balance		
		std::string dbBalanceString;
		Poco::DateTime balanceDate;
		select.reset(dbSession);
		select << "select CONVERT(balance, varchar(45)), balance_date from transactions_temp where user_id = ? order by balance_date DESC LIMIT 1",
			use(mUserId), into(dbBalanceString), into(balanceDate);

		if (!select.execute()) {
			logSuccess(dbTansactions.size());
			mIdentical = true;
			return 0;
		}

		std::string balanceDateString = Poco::DateTimeFormatter::format(balanceDate, Poco::DateTimeFormat::SORTABLE_FORMAT);
		auto nodeBalanceString = gradidoNodeRPC::getAddressBalance(mUserPublicKeyHex, balanceDateString, mGroupAlias);
		mpfr_set_str(*nodeBalance, nodeBalanceString.data(), 10, gDefaultRound);
		mpfr_set_str(*dbBalance, dbBalanceString.data(), 10, gDefaultRound);
		if (!isDecimalIdentical(*nodeBalance, *dbBalance, "end balance")) {
			return 0;
		}
		logSuccess(dbTansactions.size());
		mIdentical = true;

		return 0;
	}

	const char* CompareApolloWithGradidoNode::getNodeTransactionType(const model::gradido::TransactionBody* body)
	{
		return model::gradido::TransactionBase::getTransactionTypeString(body->getTransactionType());
	}
	const char* CompareApolloWithGradidoNode::getDBTransactionType(uint8_t typeId)
	{
		/* from apollo enums 
		
			export enum TransactionTypeId {
			  CREATION = 1,
			  SEND = 2,
			  RECEIVE = 3,
			  // This is a virtual property, never occurring on the database
			  DECAY = 4,
			  LINK_SUMMARY = 5,
			}
		*/
		switch (typeId) {
		case 1: return "CREATION";
		case 2: return "SEND";
		case 3: return "RECEIVE";
		case 4: return "DECAY";
		case 5: return "LINK_SUMMARY";
		}
		return "<unknown>";
	}
	bool CompareApolloWithGradidoNode::isDecimalIdentical(mpfr_ptr value1, mpfr_ptr value2, const char* decimalName /*= "balance"*/)
	{
		auto diff = MathMemory::create();
		mpfr_sub(*diff, value1, value2, gDefaultRound);
		mpfr_abs(*diff, *diff, gDefaultRound);
		if (mpfr_cmp_d(*diff, 0.000000000000001) > 0) {
			mErrorString = decimalName;
			mErrorString += " are not identical enough\nvalue1: ";
			std::string tempString;
			model::gradido::TransactionBase::amountToString(&tempString, value1);
			mErrorString += tempString + ",\nvalue2: ";
			model::gradido::TransactionBase::amountToString(&tempString, value2);
			mErrorString += tempString + ",\ndiff  : ";
			model::gradido::TransactionBase::amountToString(&tempString, *diff);
			mErrorString += tempString;
			return false;
		}
		return true;
	}

	void CompareApolloWithGradidoNode::logSuccess(int transactionCount)
	{
		Poco::Logger& errorLog = Poco::Logger::get("errorLog");
		if (transactionCount) {
			//errorLog.information("%d transactions from user: %u are identical on gradido node and apollo ", transactionCount, (unsigned)mUserId);
		}
		mIdentical = true;
	}

}