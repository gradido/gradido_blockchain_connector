#include "LoginServer.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/crypto/CryptoConfig.h"
#include "ConnectionManager.h"

#include "../table/BaseTable.h"
#include "../../task/RecoverLoginKeyPair.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace import {

		

		LoginServer::LoginServer()
		{

		}

		LoginServer::~LoginServer()
		{
			printf("[LoginServer::~LoginServer] \n");
		}

		void LoginServer::loadAll(const std::string& groupAlias)
		{
			if (mLoadState) return;
			mLoadState++;
			Profiler timeUsedAll;
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			Poco::Logger& speedLog = Poco::Logger::get("speedLog");

			// loading all users into memory
			/*Profiler loadingUsersTime;					
			typedef Poco::Tuple<uint64_t, std::string, std::string, std::string> UsersTuple;
			std::list<UsersTuple> users;

			select << "SELECT id, pubkey, privkey"
				<< " from users ",
				into(users);

			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load users", "users", "");
			}
			speedLog.information("loaded %d user from login server table in: %s", users.size(), loadingUsersTime.string());
			*/
			// loading all user backups into memory
			Profiler loadingUserBackupsTime;
			typedef Poco::Tuple<uint64_t, std::string, Poco::DateTime> UserBackupsTuple;
			std::list<UserBackupsTuple> userBackupsList;
			
			select << "SELECT ub.id, ub.passphrase, u.created from user_backups as ub INNER JOIN users as u on(u.id = ub.user_id)",
				into(userBackupsList);
			if (!select.execute()) {
				throw table::RowNotFoundException("couldn't load user backups", "user_backups", "");
			}

			speedLog.information("loaded %u user backups from login server table in: %s", (unsigned)userBackupsList.size(), loadingUserBackupsTime.string());
			/*
			// sorting user backups by user id
			Profiler sortUserBackupsTime;
			std::map<uint64_t, std::string> userBackupsMap;

			std::for_each(userBackupsList.begin(), userBackupsList.end(), [&](const UserBackupsTuple& userBackup) {
				userBackupsMap.insert({ userBackup.get<0>(), std::move(userBackup.get<1>()) });
			});
			userBackupsList.clear();
			speedLog.information("put user backups into map, clear user backup list in: %s", sortUserBackupsTime);
			*/
			// recover key pairs for users
			std::scoped_lock _lock(mWorkMutex);
			std::for_each(userBackupsList.begin(), userBackupsList.end(), [&](const UserBackupsTuple& userBackup) {
				task::TaskPtr task = new task::RecoverLoginKeyPair(
					userBackup.get<0>(), 
					userBackup.get<1>(),
					Poco::AutoPtr<LoginServer>(this, true),
					userBackup.get<2>(),
					groupAlias
					);
				task->scheduleTask(task);
				mRecoverKeyPairTasks.push_back(task);
			});
			//mLoadState++;
		}

		bool LoginServer::isAllRecoverKeyPairTasksFinished()
		{
			std::scoped_lock _lock(mWorkMutex);
			int erasedCount = 0;
			for (auto it = mRecoverKeyPairTasks.begin(); it != mRecoverKeyPairTasks.end(); ++it) {
				if ((*it)->isTaskFinished()) {
					it = mRecoverKeyPairTasks.erase(it);
					erasedCount++;
					if (it == mRecoverKeyPairTasks.end()) break;
				}
				else {
					return false;
				}
			}
			if (erasedCount) {
				mLoadState++;
			}
			return true;
		}
		void LoginServer::cleanTransactions()
		{
			mRecoverKeyPairTasks.clear();
		}
		bool LoginServer::addUserKey(std::unique_ptr<KeyPairEd25519> keyPair)
		{
			std::scoped_lock _lock(mWorkMutex);
			auto it = mUserKeys.insert({ std::move(keyPair->getPublicKeyHex().substr(0,64)), std::move(keyPair) });
			if (!it.second) {
				int zahl = 1;
			}
			return it.second;
		}

		const KeyPairEd25519* LoginServer::findUserKey(const std::string& pubkeyHex)
		{
			std::shared_lock _lock(mWorkMutex);
			auto it = mUserKeys.find(pubkeyHex.substr(0, 64));
			if (it == mUserKeys.end()) {
				return nullptr;
			}
			return it->second.get();
		}

		KeyPairEd25519* LoginServer::findReserveKeyPair(const unsigned char* pubkey)
		{
			for (auto it = mReserveKeyPairs.begin(); it != mReserveKeyPairs.end(); it++)
			{
				if (it->second->isTheSame(pubkey)) {
					return it->second.get();
				}
			}
			return nullptr;
		}

		const KeyPairEd25519* LoginServer::getOrCreateKeyPair(const std::string& originalPubkeyHex, const std::string& groupAlias)
		{
			std::scoped_lock _lock(mWorkMutex);
			auto senderPubkeyIt = mUserKeys.find(originalPubkeyHex);
			if (senderPubkeyIt == mUserKeys.end()) {
				return getReserveKeyPair(originalPubkeyHex, groupAlias);
			}
			return senderPubkeyIt->second.get();
		}

		LoginServer::UserTuple LoginServer::getUserInfos(const std::string& pubkeyHex)
		{
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			Poco::Data::Statement select(dbSession);
			UserTuple result; 

			select << "select id, email, first_name, last_name, created from users where hex(pubkey) LIKE ?",
				useRef(pubkeyHex), into(result), now;

			return std::move(result);
		}

		const KeyPairEd25519* LoginServer::getReserveKeyPair(const std::string& originalPubkeyHex, const std::string& groupAlias)
		{
			std::string pubkexHex = originalPubkeyHex.substr(0, 64);
			auto it = mReserveKeyPairs.find(pubkexHex);
			if (it == mReserveKeyPairs.end()) {
				auto keyPair = createReserveKeyPair(groupAlias);
				it = mReserveKeyPairs.insert({ pubkexHex, std::move(keyPair) }).first;

				Poco::Logger::get("errorLog").information("(%u) replace publickey: %s with: %s",
					(unsigned)mReserveKeyPairs.size(), pubkexHex, it->second->getPublicKeyHex());
			}

			return it->second.get();
		}
		std::unique_ptr<KeyPairEd25519> LoginServer::createReserveKeyPair(const std::string& groupAlias, Poco::DateTime created /*= Poco::DateTime(2019, 10, 8, 10)*/)
		{
			auto passphrase = Passphrase::generate(&CryptoConfig::g_Mnemonic_WordLists[CryptoConfig::MNEMONIC_BIP0039_SORTED_ORDER]);
			auto keyPair = KeyPairEd25519::create(passphrase);
			auto userPubkey = keyPair->getPublicKeyCopy();
			auto mm = MemoryManager::getInstance();
			auto tm = TransactionsManager::getInstance();

			auto registerAddress = TransactionFactory::createRegisterAddress(userPubkey, proto::gradido::RegisterAddress_AddressType_HUMAN);
			//int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int millisecond = 0, int microsecond = 0
			registerAddress->setCreated(created);
			registerAddress->updateBodyBytes();
			auto sign = keyPair->sign(*registerAddress->getTransactionBody()->getBodyBytes());
			registerAddress->addSign(userPubkey, sign);
			mm->releaseMemory(sign);
			mm->releaseMemory(userPubkey);
			tm->pushGradidoTransaction(groupAlias, std::move(registerAddress));

			return std::move(keyPair);
		}

		void LoginServer::putReserveKeysIntoDb()
		{
			/*
			// insert reserve key into db
			auto dbSession = ConnectionManager::getInstance()->getConnection();
			// load user id from users table
			Poco::Data::Statement select(dbSession);
			std::string email;
			uint64_t userId = 0;

			dbSession << "SELECT email from state_users where hex(public_key) LIKE ?",
				into(email), use(pubkexHex), now;

			if (email.size())
			{
				select.reset(dbSession);
				select << "SELECT id from users where email LIKE ?", use(email), into(userId), now;
				if (userId) {
					Poco::Data::Statement insert(dbSession);
					//insert << "INSERT INTO user_backups(user_id, passphrase, mnemonic_type) VALUES(?,?,2)"
						//, use(userId), useRef(passphrase->getString()), now;
				}
			}
			*/
		}
	}

}