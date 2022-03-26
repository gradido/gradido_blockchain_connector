#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_SESSION_MANAGER_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_SESSION_MANAGER_H

class SessionManager
{
public:
	~SessionManager();

	static SessionManager* getInstance();
protected:
	SessionManager();
};

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_SESSION_MANAGER_H