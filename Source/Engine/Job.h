#pragma once

#include <functional>

namespace Job {
	typedef std::function<void(void*)> WorkFunction;
	enum class WorkPriority
	{
		TOP_OF_QUEUE	= 0,
		NORMAL			= 50,
		BOTTOM_OF_QUEUE = 100
	};
	enum class WorkState
	{
		QUEUED, //no work started
		STARTED, //work started
		MAIN_DONE, // main work finished, preparing finish work
		FINISHING_MAIN, // queued to finish on main thread
		FINISHING, // finishing on thread
		FINISHED // work done, cleaning up
	};
	struct WorkHandle {
		bool mIsDone		 = false;
		class Work* mWorkRef = nullptr;
	};
	struct Work {
		//runs the work on whatever thread calls it
		void DoWork(bool aOnlyFinish = false);

		WorkFunction mWorkPtr	= nullptr;
		WorkFunction mFinishPtr = nullptr;

		//should we finish this job on the main thread?
		//before game loop
		bool mFinishOnMainThread = false;

		void* mUserData = nullptr;

		//somewhat useless since once it hits the finished state this is deleted
		WorkState mWorkState = WorkState::QUEUED;

		WorkHandle* mHandle = nullptr;
	};

	void QueueWork(Work& aWork, WorkPriority aWorkPriority = WorkPriority::BOTTOM_OF_QUEUE);
	//make sure to reset the handle when finished
	WorkHandle* QueueWorkHandle(Work& aWork, WorkPriority aWorkPriority = WorkPriority::BOTTOM_OF_QUEUE);

	//checks if the current thread was the thread that created the job system
	bool IsMainThread();

	//locks work mutex's to check how much work is still in the lists
	//does not include active work
	int GetWorkRemaining();

	//sleeps by using a while loop
	void SpinSleep(float aLength);

	namespace Worker {
		void Startup();
		void ProcessMainThreadWork();
		void Shutdown();

		//temp for imgui thread testing
		int GetWorkCompleted();
		double GetWorkLength();
	}; // namespace Worker

}; // namespace Job
