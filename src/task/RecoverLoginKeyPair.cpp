#include "RecoverLoginKeyPair.h"
#include "ServerConfig.h"

namespace task {
	RecoverLoginKeyPair::RecoverLoginKeyPair(uint64_t id, std::string passphrase, Poco::AutoPtr<model::import::LoginServer> loginServer)
		: CPUTask(ServerConfig::g_WorkerThread), mId(id), mPassphrase(std::move(passphrase)), mLoginServer(loginServer)
	{
#ifdef _UNI_LIB_DEBUG
		setName(std::to_string(mId).data());
#endif
	}
	RecoverLoginKeyPair::~RecoverLoginKeyPair()
	{

	}

	int RecoverLoginKeyPair::run()
	{
		auto mnemonic = Passphrase::detectMnemonic(mPassphrase);
		if (!mnemonic) {
			Poco::Logger::get("errorLog").error("couldn't find correct word list for: %u (%s)", (unsigned)mId, mPassphrase);
			return -1;
		}
		else {
			auto passphrase = std::make_shared<Passphrase>(mPassphrase, mnemonic);
			auto keyPair = KeyPairEd25519::create(passphrase);
			if (!keyPair) {
				Poco::Logger::get("errorLog").error("couldn't create key pair for: %u (%s)", (unsigned)mId, mPassphrase);
				return -2;
			}
			if (mLoginServer->addUserKeys(std::move(keyPair))) {
				return 0;
			}
			else {
				Poco::Logger::get("errorLog").error("couldn't add user keys to login server: %u (%s)", (unsigned)mId, mPassphrase);
				return -3;
			}
		}
		return 0;
	}

}