#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_GRADIDO_NODE_RPC_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_GRADIDO_NODE_RPC_H

#include "gradido_blockchain/GradidoBlockchainException.h"
#include <vector>

namespace gradidoNodeRPC
{
	std::string getAddressBalance(
		const std::string& pubkeyHex, 
		const std::string& dateString, 
		const std::string& groupAlias,
		const std::string& coinGroupId = ""
	);

	std::string getAddressType(const std::string& pubkeyHex, const std::string& groupAlias);

	std::vector<uint64_t> getAddressTxids(const std::string& pubkeyHex, const std::string& groupAlias);

	//! \return transaction serialized as base64 string
	std::string getLastTransaction(const std::string& groupAlias);
	uint64_t getLastTransactionNr(const std::string& groupAlias);

	std::string getCreationSumForMonth(
		const std::string& pubkeyHex,
		int month,
		int year,
		const std::string& startDateString,
		const std::string& groupAlias
	);
	std::vector<std::string> getTransactionsForUser(
		const std::string& pubkeyHex,
		const std::string& groupAlias,
		uint32_t maxResultCount,
		uint64_t firstTransactionNr = 1
	);
	double putTransaction(const std::string& base64Transaction, uint64_t transactionNr, const std::string& groupAlias);

	class GradidoNodeRPCException : public GradidoBlockchainException
	{
	public:
		explicit GradidoNodeRPCException(const char* what, const std::string& details = "") noexcept;
		std::string getFullString() const;
	protected:
		std::string mDetails;
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_GRADIDO_NODE_RPC_H