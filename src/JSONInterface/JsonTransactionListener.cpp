#include "JsonTransactionListener.h"
#include "model/PendingTransactions.h"
#include "model/table/Group.h"
#include "model/table/TransactionClientDetail.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/model/protobufWrapper/GradidoBlock.h"
#include "ConnectionManager.h"

using namespace rapidjson;

Document JsonTransactionListener::handle(const Document& params)
{
	std::string transactionBase64, error, iotaMessageId;
	getStringParameter(params, "transactionBase64", transactionBase64);
	getStringParameter(params, "error", error);
	getStringParameter(params, "messageId", iotaMessageId);

	if (transactionBase64.size()) {
		auto pt = model::PendingTransactions::getInstance();
		 auto bin = DataTypeConverter::base64ToBinString(transactionBase64);
		 if (error.size()) {
			 pt->updateTransaction(iotaMessageId, false, error);
		 }
		 else {
			 model::gradido::GradidoBlock gradidoBlock(&bin);
			 auto transactionBody = gradidoBlock.getGradidoTransaction()->getTransactionBody();
			 pt->updateTransaction(gradidoBlock.getMessageIdHex(), true);

			 auto iotaMessageBin = DataTypeConverter::hexToBinString(iotaMessageId)->substr(0, 32);
			 auto transactionClientDetail = model::table::TransactionClientDetail::findByIotaMessageId(iotaMessageId);
			 assert(transactionClientDetail.size() <= 1);
			 if (transactionClientDetail.size() == 1) {
				transactionClientDetail[0]->setTransactionNr(gradidoBlock.getID());
				transactionClientDetail[0]->setTxHash(gradidoBlock.getTxHash());
				transactionClientDetail[0]->save(ConnectionManager::getInstance()->getConnection());
			 }
		 }		 
	}

	return stateSuccess();
}

