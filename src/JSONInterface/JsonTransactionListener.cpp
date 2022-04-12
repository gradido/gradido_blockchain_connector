#include "JsonTransactionListener.h"
#include "model/PendingTransactions.h"
#include "model/table/Group.h"
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
			 if (transactionBody->isGlobalGroupAdd()) {
				 auto globalGroupAdd = transactionBody->getGlobalGroupAdd();
				 std::unique_ptr<model::table::Group> group;
				 try {
					 group = model::table::Group::load(globalGroupAdd->getGroupAlias());
					 group->setName(globalGroupAdd->getGroupName());
					 group->setCoinColor(globalGroupAdd->getCoinColor());
				 }
				 catch (model::table::RowNotFoundException& ex) {
					 group = std::make_unique<model::table::Group>(
						 globalGroupAdd->getGroupName(),
						 globalGroupAdd->getGroupAlias(),
						 "",
						 globalGroupAdd->getCoinColor()
						 );
				 }
				 auto dbSession = ConnectionManager::getInstance()->getConnection();
				 group->save(dbSession);
			 }
			 pt->updateTransaction(gradidoBlock.getMessageIdHex(), true);
		 }
	}

	return stateSuccess();
}

