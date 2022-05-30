#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H

#include <map>
#include <string>
#include "../table/BaseTable.h"
#include "gradido_blockchain/crypto/KeyPairEd25519.h"
#include "gradido_blockchain/MemoryManager.h"

namespace model {
	namespace import {
		class LoginServer;

		class CommunityServer
		{
		public:
			CommunityServer();
			~CommunityServer();

			typedef Poco::Tuple<Poco::UInt64, Poco::DateTime, Poco::UInt64> StateBalanceTuple;
			struct StateBalance
			{
				StateBalance(const StateBalanceTuple& tuple);
				Poco::DateTime recordDate;
				Poco::UInt64 amountGddCent;
			};
			//state_user_id, transaction_id, balance, balance_date
			typedef Poco::Tuple<Poco::UInt64, Poco::UInt64, Poco::UInt64, Poco::DateTime> StateUserTransactionTuple;
			struct StateUserTransaction
			{
				StateUserTransaction(const StateUserTransactionTuple& tuple);
				Poco::UInt64 userId;
				Poco::UInt64 transactionId;
				Poco::UInt64 balanceGddCent;
				Poco::DateTime balanceDate;
			};

			void loadStateUsers();
			//! \param userKeys for signing transactions
			void loadTransactionsIntoTransactionManager(const std::string& groupAlias, const std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>>* userKeys = nullptr);
			void loadStateUserBalances();
			void loadAll(const std::string& groupAlias, bool shouldLoadStateUserBalances = false, Poco::AutoPtr<LoginServer> loginServer = nullptr);
	
			inline const std::map<Poco::UInt64, StateBalance>& getStateBalances() { return mStateBalances; }
			inline const std::map<Poco::UInt64, std::map<Poco::UInt64, StateUserTransaction>>& getStateUserTransactions() { return mStateUserTransactions; }
			std::string getUserPubkeyHex(Poco::UInt64 userId);
			KeyPairEd25519* findReserveKeyPair(const unsigned char* pubkey);

			class UserIdNotFoundException : public GradidoBlockchainException
			{
			public: 
				explicit UserIdNotFoundException(const char* what, Poco::UInt64 userId) noexcept
					: GradidoBlockchainException(what), mUserId(userId) {};

				std::string getFullString() const { return std::string(what()) +  std::string(", user id: ") + std::to_string(mUserId); }
			protected:
				Poco::UInt64 mUserId;
			};


		protected:
			
			MemoryBin* getUserPubkey(uint64_t userId, uint64_t transactionId);
			KeyPairEd25519* getReserveKeyPair(const std::string& originalPubkeyHex);

			std::map<uint64_t, std::string> mStateUserIdPublicKey;
			std::unordered_map<std::string, uint64_t> mPublicKeyStateUserId;
			//! key is original user pubkey hex
			std::unordered_map<std::string, std::unique_ptr<KeyPairEd25519>> mReserveKeyPairs;
			//! key is user id
			std::map<Poco::UInt64, StateBalance> mStateBalances;
			//! first key is user id, seconds key is transaction id
			std::map<Poco::UInt64, std::map<Poco::UInt64, StateUserTransaction>> mStateUserTransactions;
			Poco::AtomicCounter mLoadState;
		};
	}
}


#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H