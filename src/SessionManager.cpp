#include "SessionManager.h"

SessionManager::SessionManager()
{

}

SessionManager::~SessionManager()
{

}

SessionManager* SessionManager::getInstance()
{
	static SessionManager one;
	return &one;
}