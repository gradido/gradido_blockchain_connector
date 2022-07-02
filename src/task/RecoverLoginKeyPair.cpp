#include "RecoverLoginKeyPair.h"
#include "ServerConfig.h"
#include "gradido_blockchain/model/TransactionsManager.h"
#include "gradido_blockchain/model/TransactionFactory.h"
#include "gradido_blockchain/MemoryManager.h"

namespace task {
	RecoverLoginKeyPair::RecoverLoginKeyPair(
		uint64_t id, 
		std::string passphrase, 
		Poco::AutoPtr<model::import::LoginServer> loginServer,
		Poco::DateTime created,
		const std::string& groupAlias
	)
		: CPUTask(ServerConfig::g_WorkerThread), mId(id), 
		mPassphrase(std::move(passphrase)), mLoginServer(loginServer),
		mCreated(created), mGroupAlias(groupAlias)
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

			if (mOriginalPubkey.size()) {
				auto publicHex = keyPair->getPublicKeyHex();
				if (publicHex.substr(0, 64) != mOriginalPubkey.substr(0, 64)) {
					Poco::Logger::get("errorLog").error("original pubkey differ from recovered pubkey for %u", (unsigned)mId);
					Poco::Logger::get("errorLog").error("%s != %s", publicHex, mOriginalPubkey);
					return -4;
				}
			}

			auto mm = MemoryManager::getInstance();
			auto tm = model::TransactionsManager::getInstance();
			auto pubkeyCopy = keyPair->getPublicKeyCopy();
			
			auto registerAddress = TransactionFactory::createRegisterAddress(pubkeyCopy, proto::gradido::RegisterAddress_AddressType_HUMAN);
			registerAddress->setCreated(mCreated);
			registerAddress->updateBodyBytes();
			auto sign = keyPair->sign(*registerAddress->getTransactionBody()->getBodyBytes());
			registerAddress->addSign(pubkeyCopy, sign);
			mm->releaseMemory(sign);
			mm->releaseMemory(pubkeyCopy);

			if (mLoginServer->addUserKey(std::move(keyPair))) {
				try {
					tm->pushGradidoTransaction(mGroupAlias, std::move(registerAddress));
				}
				catch (std::exception& ex) {
					printf("exception: %s\n", ex.what());
				}
				return 0;
			}
			else {
				Poco::Logger::get("errorLog").error("couldn't add user keys to login server: %u (%s)", (unsigned)mId, mPassphrase);
				if (mOriginalPubkey.size()) {
					auto findKeyPair = mLoginServer->findUserKey(mOriginalPubkey);
					if (findKeyPair) {
						Poco::Logger::get("errorLog").error("key pair was already added: %s", mOriginalPubkey);
					}
				}

				mm->releaseMemory(pubkeyCopy);
				return -3;
			}
		}
		return 0;
	}

}