#include "Task.h"

namespace task {
	
	Task::Task() 
	: mTaskScheduled(false), mFinishCommand(nullptr), mParentTaskPtrArray(nullptr),
		mParentTaskPtrArraySize(0), mDeleted(false), mFinished(false), mReferenceCount(1)
	{
	}

    Task::Task(size_t taskPointerArraySize)
        : mTaskScheduled(false), mFinishCommand(nullptr), mParentTaskPtrArray(new TaskPtr[taskPointerArraySize]), mParentTaskPtrArraySize(taskPointerArraySize),
            mDeleted(false), mFinished(false), mReferenceCount(1)
    {
    }
		
	Task::~Task()
	{
		mWorkingMutex.lock();
		//printf("[Task::~Task]\n");
		if (mParentTaskPtrArraySize) {
			delete[] mParentTaskPtrArray;
			mParentTaskPtrArray = nullptr;
		}
		if (mFinishCommand) {
			delete mFinishCommand;
			mFinishCommand = nullptr;
		}
        mParentTaskPtrArraySize = 0;
			
        mDeleted = true;
		//printf("[Task::~Task] finished\n");
		mWorkingMutex.unlock();
			
	}

    bool Task::isAllParentsReady()
    {
        bool allFinished = true;
		lock();
        for(size_t i = 0; i < mParentTaskPtrArraySize; i++) {
            TaskPtr task = mParentTaskPtrArray[i];
			if (!task.isNull() && !task->isTaskFinished()) {
                allFinished = false;
                if(!task->isTaskSheduled()) 
                    mParentTaskPtrArray[i]->scheduleTask(mParentTaskPtrArray[i]);
            }
        }
		unlock();
        return allFinished;
    }

	TaskPtr Task::getParent(int index)
	{
		if (index < 0 || index >= mParentTaskPtrArraySize) {
			return nullptr;
		}
		return mParentTaskPtrArray[index];
	}

	void Task::duplicate()
	{
		Poco::FastMutex::ScopedLock _lock(mReferenceMutex);
		//mReferenceMutex.lock();
		mReferenceCount++;
		//printf("[Task::duplicate] new value: %d\n", mReferenceCount);
		//mReferenceMutex.unlock();
	}

	void Task::release()
	{
		mReferenceMutex.lock();
		
		mReferenceCount--;
		//printf("[Task::release] new value: %d\n", mReferenceCount);
		if (0 == mReferenceCount) {
			mReferenceMutex.unlock();
			delete this;
			return;
		}
		mReferenceMutex.unlock();
	}
	int Task::getReferenceCount()
	{
		Poco::FastMutex::ScopedLock _lock(mReferenceMutex);
		return mReferenceCount;
	}

	void Task::lock()
	{
		mWorkingMutex.lock(500);
	}

	void Task::setTaskFinished() {
		lock(); 
		mFinished = true; 
		if (mFinishCommand) {
			mFinishCommand->taskFinished(this);
		}
		unlock(); 
	}
}