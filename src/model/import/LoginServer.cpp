#include "LoginServer.h"
#include "gradido_blockchain/lib/Profiler.h"
#include "ConnectionManager.h"
#include "../table/BaseTable.h"

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
			Profiler recoverKeyPairsTime;
			std::for_each(userBackupsList.begin(), userBackupsList.end(), [&](const UserBackupsTuple& userBackup) {
				auto passphraseString = userBackup.get<1>();
				auto mnemonic = Passphrase::detectMnemonic(passphraseString);
				if (!mnemonic) {
					Poco::Logger::get("errorLog").error("couldn't find correct word list for: %u (%s)", (unsigned)userBackup.get<0>(), passphraseString);
				}
				else {
					auto passphrase = std::make_shared<Passphrase>(passphraseString, mnemonic);
					auto keyPair = KeyPairEd25519::create(passphrase);
					mUserKeys.insert({ keyPair->getPublicKeyHex(), std::move(keyPair) });
				}
			});
			speedLog.information("recover: %u key pairs in %s", (unsigned)mUserKeys.size(), recoverKeyPairsTime.string());
			speedLog.information("[LoginServer::loadAll] time: %s", timeUsedAll.string());
		}
	}

}