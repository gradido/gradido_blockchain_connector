#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_OPEN_CONNECTION_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_OPEN_CONNECTION_H

#include "gradido_blockchain/http/JsonRequestHandler.h"

/*!
 @author einhornimmond
 @date 21.09.2022
 @brief Open Connection Call from Apollo Cross Group Communication Protocol


@startuml Example of communication between Apollo and Blockchain-Connector
Apollo -> BlockchainConnector: openCommunication(community-key-A, signature) )
BlockchainConnector --> Apollo: JWT
Apollo -> BlockchainConnector: login(name, password) (JWT in header)
BlockchainConnector --> Apollo: JWT (contain username)
Apollo -> BlockchainConnector: transfer (JWT with sender user in header)
BlockchainConnector --> Apollo: iotaMessageId on success
@enduml
*/

class JsonOpenConnection : public JsonRequestHandler
{
public:
	void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);

};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_JSON_INTERFACE_OPEN_CONNECTION_H