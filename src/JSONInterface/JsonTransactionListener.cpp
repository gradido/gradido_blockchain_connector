#include "JsonTransactionListener.h"
#include "model/PendingTransactions.h"
#include "model/table/Group.h"
#include "model/table/TransactionClientDetail.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"

using namespace rapidjson;

Document JsonTransactionListener::handle(const Document& params)
{
	std::string transactionBase64, error, iotaMessageId;
	getStringParameter(params, "transactionBase64", transactionBase64);
	getStringParameter(params, "error", error);
	getStringParameter(params, "messageId", iotaMessageId);

	if (transactionBase64.size()) {
		auto pt = model::PendingTransactions::getInstance();
		 
		 std::unique_ptr<model::gradido::GradidoBlock> gradidoBlock;
		 std::string iotaMessageBin;
		 if (error.size()) {
			 pt->updateTransaction(iotaMessageId, false, error);
			 iotaMessageBin = DataTypeConverter::hexToBinString(iotaMessageId)->substr(0, 32);
		 }
		 else {
			 auto bin = DataTypeConverter::base64ToBinString(transactionBase64);
			 gradidoBlock = std::make_unique<model::gradido::GradidoBlock>(&bin);
			 pt->updateTransaction(gradidoBlock->getMessageIdHex(), true);
			 iotaMessageBin = gradidoBlock->getMessageIdString();
		 }
		 auto transactionClientDetails = model::table::TransactionClientDetail::findByIotaMessageId(iotaMessageBin);

		 for (auto it = transactionClientDetails.begin(); it != transactionClientDetails.end(); ++it) {
			 auto t = it->get();
			 if (error.size()) {
				 t->setPendingState(model::PendingTransactions::PendingTransaction::State::REJECTED);
			 }
			 else {
				 t->setPendingState(model::PendingTransactions::PendingTransaction::State::CONFIRMED);
				 t->setTransactionNr(gradidoBlock->getID());
				 t->setTxHash(gradidoBlock->getTxHash());
			 }
			 t->setError(error);
			 
			 t->save();
		 }

			 
	}

	return stateSuccess();
}

