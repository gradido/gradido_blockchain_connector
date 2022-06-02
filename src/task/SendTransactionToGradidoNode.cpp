#include "SendTransactionToGradidoNode.h"
#include "../ServerConfig.h"
#include "../GradidoNodeRPC.h"
#include "gradido_blockchain/model/TransactionsManagerBlockchain.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/protobufWrapper/TransactionValidationExceptions.h"

#include "Poco/Util/ServerApplication.h"

namespace task {
	SendTransactionToGradidoNode::SendTransactionToGradidoNode(
		std::shared_ptr<model::gradido::GradidoTransaction> transaction,
		uint64_t transactionNr,
		const std::string& groupAlias,
		MultithreadQueue<double>* runtime
	) : CPUTask(ServerConfig::g_WorkerThread), mTransaction(transaction), mTransactionNr(transactionNr), mGroupAlias(groupAlias), mRuntime(runtime)
	{
#ifdef _UNI_LIB_DEBUG
		setName(mTransaction->toJson().data());
#endif

	}
	int SendTransactionToGradidoNode::run()
	{
		Poco::Logger& errorLog = Poco::Logger::get("errorLog");
		auto tm = model::TransactionsManager::getInstance();
		try {
			auto transactionCopy = std::make_unique<model::gradido::GradidoTransaction>(mTransaction->getSerialized().get());
			auto gradidoBlock = model::gradido::GradidoBlock::create(
				std::move(transactionCopy),
				mTransactionNr,
				mTransaction->getTransactionBody()->getCreatedSeconds(),
				nullptr
			);

			auto level = static_cast<model::gradido::TransactionValidationLevel>(model::gradido::TRANSACTION_VALIDATION_SINGLE |
				model::gradido::TRANSACTION_VALIDATION_DATE_RANGE |
				model::gradido::TRANSACTION_VALIDATION_SINGLE_PREVIOUS
			);

			model::TransactionsManagerBlockchain blockchain(mGroupAlias);
			mTransaction->validate(
				level,
				&blockchain,
				gradidoBlock
			);
		}
		catch (model::gradido::TransactionValidationInvalidSignatureException& ex) {
			errorLog.error("invalid signature: %s", ex.getFullString());
			auto bodyBytes = mTransaction->getTransactionBody()->getBodyBytes();
			auto bodyBytesBase64 = DataTypeConverter::binToBase64(*bodyBytes);
			errorLog.error("body bytes now: %s", bodyBytesBase64);
			return -1;
		}
		catch (model::gradido::TransactionValidationForbiddenSignException& ex) {
			errorLog.error("forbidden sign: %s", ex.getFullString());
			auto transactionBody = mTransaction->getTransactionBody();
			if (transactionBody->isCreation()) {
				auto pubkey = mTransaction->getTransactionBody()->getCreationTransaction()->getRecipientPublicKeyString();
				auto pubkeyHex = DataTypeConverter::binToHex(pubkey);
				errorLog.error("recipient pubkey: %s", pubkeyHex);
			}
			return -2;
		}
		catch (model::gradido::TransactionValidationException& ex) {
			errorLog.error("error validating transaction %d: %s", (int)mTransactionNr, ex.getFullString());
			errorLog.error("transaction in json: %s", mTransaction->toJson());

			std::string pubkeyHex;
			auto transactionBody = mTransaction->getTransactionBody();
			if (transactionBody->isCreation()) {
				auto pubkey = mTransaction->getTransactionBody()->getCreationTransaction()->getRecipientPublicKeyString();
				pubkeyHex = DataTypeConverter::binToHex(pubkey).substr(0,64);
			}
			else if (transactionBody->isTransfer()) {
				auto pubkey = mTransaction->getTransactionBody()->getTransferTransaction()->getSenderPublicKeyString();
				pubkeyHex = DataTypeConverter::binToHex(pubkey).substr(0, 64);
			}			
			try {
				errorLog.error("transactions for user: %s", tm->getUserTransactionsDebugString(mGroupAlias, pubkeyHex));
			}
			catch (GradidoBlockchainException& ex) {
				errorLog.error("couldn't create debug string for user: %s", pubkeyHex);
				ServerConfig::g_WorkerThread->stop();
				Poco::Util::ServerApplication::terminate();
			}
			ServerConfig::g_WorkerThread->stop();
			Poco::Util::ServerApplication::terminate();
			return -3;
		}
		catch (GradidoBlockchainException& ex) {
			errorLog.error("gradido blockchain exception for transaction: %d: %s", (int)mTransactionNr, ex.getFullString());
			ServerConfig::g_WorkerThread->stop();
			Poco::Util::ServerApplication::terminate();
			return -4;
		}
		auto base64Transaction = DataTypeConverter::binToBase64(std::move(mTransaction->getSerialized()));

		try {
			auto timeMilliSeconds = gradidoNodeRPC::putTransaction(*base64Transaction, mTransactionNr, mGroupAlias);
			if (mRuntime) {
				mRuntime->push(timeMilliSeconds);
			}
		}
		catch (GradidoBlockchainException& ex) {
			errorLog.error("error by sending transaction %d to gradido node: %s", (int)mTransactionNr, ex.getFullString());
			errorLog.error("transaction in json: %s", mTransaction->toJson());
			ServerConfig::g_WorkerThread->stop();
			Poco::Util::ServerApplication::terminate();
			return -4;
		}
		return 0;
	}
}