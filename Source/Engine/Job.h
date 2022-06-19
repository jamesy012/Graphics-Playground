#pragma once

namespace Job {
	typedef void (*WorkFunction)();
	typedef int Workhandle;
	struct Work {
		int value;//temp

		WorkFunction workPtr = nullptr;
		WorkFunction finishPtr = nullptr;

		//should we finish this job on the main thread?
		//before game loop
		bool finishOnMainThread = false;

		Workhandle handle = -1;
	};

	Workhandle QueueWork(Work& aWork);
	Workhandle QueueWork(int aWork);
	Workhandle QueueWork(WorkFunction aWork);

	bool IsFinished(Workhandle aHandle);

	namespace Worker {
		void Startup();
		void Shutdown();
	};

}; // namespace Job
