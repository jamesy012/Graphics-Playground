#pragma once

#include <functional>

struct WorkManager {
	static void Startup();
	static void ProcessMainThreadWork();
	static void Shutdown();

	//temp for imgui thread testing
	static int GetWorkCompleted();
	static double GetWorkLength();
};

struct Job {
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

	struct Work;
	struct Worker;

	struct WorkHandle {
		public:
		WorkState GetState() const;
	protected:
		bool mIsDone		 = false;
		class Work* mWorkRef = nullptr;

		friend Work;
		friend Worker;
		friend Job;
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
		//does work state need a mutex/atomic for access?
		WorkState mWorkState = WorkState::QUEUED;

	protected:
		WorkHandle* mHandle = nullptr;

		friend Job;
	};

	static void QueueWork(std::vector<Work>& aWork, WorkPriority aWorkPriority = WorkPriority::BOTTOM_OF_QUEUE);
	static void QueueWork(Work& aWork, WorkPriority aWorkPriority = WorkPriority::BOTTOM_OF_QUEUE);
	//make sure to reset the handle when finished
	static std::vector<WorkHandle*> QueueWorkHandle(std::vector<Work>& aWork, WorkPriority aWorkPriority = WorkPriority::BOTTOM_OF_QUEUE);
	static WorkHandle* QueueWorkHandle(Work& aWork, WorkPriority aWorkPriority = WorkPriority::BOTTOM_OF_QUEUE);

	static bool IsDone(const WorkHandle* aHandle){
		return aHandle->mIsDone;
	}

	//returns true if we waited for work to finish
	//false if handle
	static bool WaitForWork(const WorkHandle* aHandle);

	//checks if the current thread was the thread that created the job system
	static bool IsMainThread();

	//locks work mutex's to check how much work is still in the lists
	//does not include active work
	static int GetWorkRemaining();

	//sleeps by using a while loop
	static void SpinSleep(float aLength);


	private:
};
