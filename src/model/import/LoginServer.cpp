#include "LoginServer.h"
#include "gradido_blockchain/lib/Profiler.h"
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

		}

		void LoginServer::loadAll()
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
			typedef Poco::Tuple<uint64_t, std::string> UserBackupsTuple;
			std::list<UserBackupsTuple> userBackupsList;
			
			select << "SELECT id, passphrase from user_backups",
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
			
			std::for_each(userBackupsList.begin(), userBackupsList.end(), [&](const UserBackupsTuple& userBackup) {
				task::TaskPtr task = new task::RecoverLoginKeyPair(userBackup.get<0>(), userBackup.get<1>(), Poco::AutoPtr<LoginServer>(this, true));
				task->scheduleTask(task);
				mRecoverKeyPairTasks.push_back(task);
			});
			//mLoadState++;
		}

		bool LoginServer::isAllRecoverKeyPairTasksFinished()
		{
			int erasedCount = 0;
			for (auto it = mRecoverKeyPairTasks.begin(); it != mRecoverKeyPairTasks.end(); it++) {
				if ((*it)->isTaskFinished()) {
					it = mRecoverKeyPairTasks.erase(it);
					erasedCount++;
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
		bool LoginServer::addUserKeys(std::unique_ptr<KeyPairEd25519> keyPair)
		{
			std::scoped_lock _lock(mWorkMutex);
			return mUserKeys.insert({ std::move(keyPair->getPublicKeyHex()), std::move(keyPair) }).second;
		}
	}

}