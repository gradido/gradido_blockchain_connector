#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H

#include <map>
#include <string>

namespace model {
	namespace import {
		class CommunityServer
		{
		public:
			CommunityServer();
			~CommunityServer();

			void loadStateUsers();

		protected:
			std::map<uint64_t, std::string> mStateUserIdPublicKey;
		};
	}
}


#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_MODEL_IMPORT_COMMUNITY_SERVER_H