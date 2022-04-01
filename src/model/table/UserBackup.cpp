#include "UserBackup.h"

namespace model {
	namespace table {
		UserBackup::UserBackup()
			: mUserId(0), mKeyUserId(0), mEncryptedPassphrase(nullptr)
		{

		}

		UserBackup::UserBackup(uint64_t userId, uint64_t keyUserId, const MemoryBin* encryptedPassphrase)
			: mUserId(userId), mKeyUserId(keyUserId), mEncryptedPassphrase(*encryptedPassphrase->copyAsString().get())
		{

		}

		UserBackup::UserBackup(UserBackupTuple data)
			: BaseTable(std::get<0>(data)), mUserId(std::get<1>(data)), mKeyUserId(std::get<2>(data)), mEncryptedPassphrase(std::get<3>(data))
		{

		}

		UserBackup::~UserBackup()
		{

		}

		uint64_t UserBackup::save()
		{
			auto dbConnection = ServerConfig::g_Mysql->connect();
			std::stringstream sqlQuery;
			sqlQuery << "INSERT INTO " << getTableName() << "(user_id, key_user_id, encrypted_passphrase) VALUES(?,?,?)";

			auto preparedStatement = dbConnection.prepare(sqlQuery.str());
			auto result = preparedStatement(mUserId, mKeyUserId, mEncryptedPassphrase);
			return result.last_insert_id();
		}
	}
		 
}