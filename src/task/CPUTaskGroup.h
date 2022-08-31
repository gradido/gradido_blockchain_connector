#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_CPU_TASK_GROUP_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_CPU_TASK_GROUP_H

#include "CPUTask.h"
#include "Poco/AutoPtr.h"
#include <list>

namespace task {
	template<class T>
	class CPUTaskGroup
	{
	public:
		//! \param waitingTime: how many ms wait between check if all tasks already finished
		//! 
		CPUTaskGroup(long waitingTime = 10, bool removeFromListAfterFinished = true);
		~CPUTaskGroup();
		void pushTask(Poco::AutoPtr<T> _task);
		void join();
		inline const std::list<Poco::AutoPtr<T>>& getPendingTasksList() const { return mPendingTasks; }

	protected:
		std::list<Poco::AutoPtr<T>> mPendingTasks;
		long mWaitingTime;
		bool mRemoveFromListAfterFinished;
	};

	template<class T>
	CPUTaskGroup<T>::CPUTaskGroup(long waitingTime/* = 10*/, bool removeFromListAfterFinished/* = true*/)
		: mWaitingTime(waitingTime), mRemoveFromListAfterFinished(removeFromListAfterFinished)
	{

	}
	template<class T>
	CPUTaskGroup<T>::~CPUTaskGroup()
	{

	}
	template<class T>
	void CPUTaskGroup<T>::pushTask(Poco::AutoPtr<T> _task)
	{
		mPendingTasks.push_back(_task);
		_task->scheduleTask(_task);
	}
	template<class T>
	void CPUTaskGroup<T>::join()
	{
		bool isAtLeastOnTaskUnfinished;
		do
		{
			Poco::Thread::sleep(mWaitingTime);
			isAtLeastOnTaskUnfinished = false;
			for (auto it = mPendingTasks.begin(); it != mPendingTasks.end(); ++it) {
				if ((*it)->isTaskFinished()) {
					if (mRemoveFromListAfterFinished) {
						it = mPendingTasks.erase(it);
						if (it == mPendingTasks.end()) break;
					}
				}
				else {
					isAtLeastOnTaskUnfinished = true;
					break;
				}
			}

		} while (isAtLeastOnTaskUnfinished);
	}
}

#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_TASK_CPU_TASK_GROUP_H