#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H

#include <map>
#include <string>
#include "gradido_blockchain/MemoryManager.h"

namespace model {
	namespace import {
		class CommunityServer
		{
		public:
			CommunityServer();
			~CommunityServer();

			void loadStateUsers();
			void loadTransactionsIntoTransactionManager(const std::string& groupAlias);

		protected:
			MemoryBin* getUserPubkey(uint64_t userId, uint64_t transactionId);
			std::map<uint64_t, std::string> mStateUserIdPublicKey;
		};
	}
}


#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H