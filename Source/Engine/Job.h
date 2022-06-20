#pragma once

#include <functional>

namespace Job {
	typedef std::function<void()> WorkFunction;
	typedef int Workhandle;
	struct Work {
		WorkFunction workPtr = nullptr;
		WorkFunction finishPtr = nullptr;

		//should we finish this job on the main thread?
		//before game loop
		bool finishOnMainThread = false;

		Workhandle handle = -1;
	};

	Workhandle QueueWork(Work& aWork);
	Workhandle QueueWork(WorkFunction aWork);
	Workhandle QueueWork();

	bool IsFinished(Workhandle aHandle);

	bool IsMainThread();

	namespace Worker {
		void Startup();
		void ProcessMainThreadWork();
		void Shutdown();
			//temp
	int GetWorkCompleted();
	double GetWorkLength();
	};

}; // namespace Job
