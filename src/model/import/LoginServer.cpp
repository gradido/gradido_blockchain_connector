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
		bool LoginServer::addUserKeys(std::unique_ptr<KeyPairEd25519> keyPair)
		{
			std::scoped_lock _lock(mWorkMutex);
			return mUserKeys.insert({ std::move(keyPair->getPublicKeyHex().substr(0,64)), std::move(keyPair) }).second;
		}
	}

}