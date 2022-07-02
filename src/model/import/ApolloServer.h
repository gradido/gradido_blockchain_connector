#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_APOLLO_SERVER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_APOLLO_SERVER_H

#include "LoginServer.h"
#include "Poco/Tuple.h"

namespace model {
	namespace import {
		class ApolloServer: public LoginServer
		{
		public:
			ApolloServer(const std::string& groupAlias);
			~ApolloServer();

			void loadAll(const std::string& groupAlias);
			inline const KeyPairEd25519* getFounderKeyPair() const { return mFounderKeyPair.get(); }
			const KeyPairEd25519* getUserKeyPair(uint64_t userId);

			bool isAllTransactionTasksFinished();

			void cleanTransactions();

			// id, user_id, type_id, amount, balance_date, memo, creation_date, linked_user_id
			typedef Poco::Tuple<uint64_t, uint64_t, uint32_t, double, Poco::DateTime, std::string, Poco::DateTime, uint64_t> TransactionTuple;
			virtual UserTuple getUserInfos(const std::string& pubkeyHex);

		protected:
			void loadTransactions(const std::string& groupAlias);
			std::unique_ptr<KeyPairEd25519> mFounderKeyPair;
			std::map<uint64_t, std::string> mUserIdPubkeyHex;
			std::list<task::TaskPtr> mTransactionTasks;
			std::string mGroupAlias;
			Poco::Mutex mApolloWorkMutex;
		};
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_APOLLO_SERVER_H