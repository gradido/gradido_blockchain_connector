#ifndef Gradido_LoginServer_INCLUDED
#define Gradido_LoginServer_INCLUDED

#include "Poco/Util/ServerApplication.h"

class GradidoBlockchainConnector : public Poco::Util::ServerApplication
{

	/// The main application class.
	///
	/// This class handles command-line arguments and
	/// configuration files.
	/// Start the Gradido_LoginServer executable with the help
	/// option (/help on Windows, --help on Unix) for
	/// the available command line options.
	///
	

public:
	GradidoBlockchainConnector();
	~GradidoBlockchainConnector();

protected:
	void initialize(Application& self);

	void uninitialize();

	void defineOptions(Poco::Util::OptionSet& options);

	void handleOption(const std::string& name, const std::string& value);
	void displayHelp();
	void checkCommunityServerStateBalances(const std::string& groupAlias);
	void checkApolloServerDecay(const std::string& groupAlias);
	void sendArchivedTransactionsToGradidoNode(const std::string& groupAlias, bool iota = false, bool apollo = false);

	int main(const std::vector<std::string>& args);

	void createConsoleFileAsyncLogger(std::string name, std::string filePath);
	void createFileLogger(std::string name, std::string filePath);

private:
	bool _helpRequested;

	enum class ArchivedTransactionsHandling : int {
		NONE = 0,
		USE_COMMUNITY_SERVER_DATA = 1,
		USE_APOLLO_SERVER_DATA = 2,
		CHECK_DATA = 4,
		SEND_DATA_TO_GRADIDO_NODE = 8,
		SEND_DATA_TO_IOTA = 16
	};

	ArchivedTransactionsHandling mArchivedTransactionsHandling;
	std::string mConfigPath;
};

#endif //Gradido_LoginServer_INCLUDED
