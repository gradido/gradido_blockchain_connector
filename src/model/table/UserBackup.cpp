#include "UserBackup.h"
#include "ConnectionManager.h"

using namespace Poco::Data::Keywords;

namespace model {
	namespace table {
		UserBackup::UserBackup()
			: mUserId(0), mKeyUserId(0)
		{

		}

		UserBackup::UserBackup(uint64_t userId, uint64_t keyUserId, const MemoryBin* encryptedPassphrase)
			: mUserId(userId), mKeyUserId(keyUserId), mEncryptedPassphrase(*encryptedPassphrase->copyAsString().get())
		{

		}

		UserBackup::UserBackup(const UserBackupTuple& data)
			: BaseTable(data.get<0>()), mUserId(data.get<1>()), mKeyUserId(data.get<2>()), mEncryptedPassphrase(data.get<3>())
		{

		}

		UserBackup::~UserBackup()
		{

		}

		void UserBackup::save(Poco::Data::Session& dbSession)
		{
			Poco::Data::Statement insert(dbSession);

			insert << "INSERT INTO " << getTableName() << "(user_id, key_user_id, encrypted_passphrase) VALUES(?,?,?);",
				use(mUserId), use(mKeyUserId), use(mEncryptedPassphrase), now;
		}
	}
		 
}