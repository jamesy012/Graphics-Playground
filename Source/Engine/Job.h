#pragma once

namespace Job {
	typedef void (*WorkFunction)();
	struct Work {
		int value;
		WorkFunction functionPtr;
	};

	void QueueWork(int aWork);
	void QueueWork(WorkFunction aWork);

	namespace Worker {
		void Startup();
		void Shutdown();
	};

}; // namespace Job
