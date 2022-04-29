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
		uint32_t coinColor = 0
	);

	std::vector<uint64_t> getAddressTxids(const std::string& pubkeyHex, const std::string& groupAlias);

	std::string getCreationSumForMonth(
		const std::string& pubkeyHex,
		int month,
		int year,
		const std::string& startDateString,
		const std::string& groupAlias
	);

	class GradidoNodeRPCException : public GradidoBlockchainException
	{
	public:
		explicit GradidoNodeRPCException(const char* what) noexcept : GradidoBlockchainException(what) {};
		std::string getFullString() const { return std::string(what()); };
	};
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_GRADIDO_NODE_RPC_H