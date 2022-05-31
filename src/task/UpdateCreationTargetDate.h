#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_UPDATE_CREATION_TARGET_DATE_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_UPDATE_CREATION_TARGET_DATE_H

#include "CPUTask.h"

namespace task {
	class UpdateCreationTargetDate : public CPUTask
	{
	public:
		UpdateCreationTargetDate(uint64_t stateUserId);
		~UpdateCreationTargetDate();

		const char* getResourceType() const { return "UpdateCreationTargetDate"; };
		int run();
	protected:
		uint64_t mStateUserId;
	};
}


#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_UPDATE_CREATION_TARGET_DATE_H