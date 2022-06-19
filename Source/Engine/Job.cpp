#include "Job.h"

#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <condition_variable>

#include "PlatformDebug.h"

struct WorkerManager {
public:
	TracyLockable(std ::mutex, mWorkAccesser);
	//std::mutex mWorkAccesser;
	std::vector<std::thread> mWorkThreads;
	std::queue<Job::Work> mWork;
	//std::atomic_char mWorkCount;
	std::condition_variable_any m_CV;
	bool mQuit = false;

} gManager;
bool WorkAvailable() {
	//return gManager.mWorkCount != 0 || gManager.mQuit == true;
	return gManager.mWork.size() != 0 || gManager.mQuit == true;
}
namespace Job {

	void AddWork(Work aWork) {
		ZoneScoped;
		std::unique_lock lock(gManager.mWorkAccesser);
		gManager.mWork.push(aWork);
		//gManager.mWorkCount++;
		gManager.m_CV.notify_one();
		lock.unlock();
		LOGGER::Formated("Work Added\n");
	}

	void QueueWork(int aWork) {
		Work work  = {};
		work.value = aWork;
		AddWork(work);
	}
	void QueueWork(WorkFunction aWork) {
		Work work		 = {};
		work.functionPtr = aWork;
		AddWork(work);
	}

	namespace Worker {

		void WorkerThread(int threadIndex) {
#if defined(TRACY_ENABLE)
			std::string text = "Worker thread: " + std::to_string(threadIndex);
			tracy::SetThreadName(text.c_str());
#endif

			LOGGER::Formated("Work setup\n");
			auto hash = std::hash<std::thread::id>();
			srand(hash(std::this_thread::get_id()));
			while(true) {
				Work work;
				{
					//get work
					std::unique_lock lock(gManager.mWorkAccesser);
					gManager.m_CV.wait(lock, WorkAvailable);
					if(gManager.mQuit) {
						break;
					}
					work = gManager.mWork.front();
					gManager.mWork.pop();
					lock.unlock();
				}
				ZoneScoped;
				LOGGER::Formated("Work Started {}\n", threadIndex);
				if(work.functionPtr) {
					ZoneScopedN("Fn Ptr");
					work.functionPtr();
				} else {
					LOGGER::Formated("\tWork value {}\n", (int)work.value);
				}
				LOGGER::Formated("Work Finished {}\n", threadIndex);
			}
			LOGGER::Formated("Work Closing\n");
		}

		void Startup() {

			// Retrieve the number of hardware threads in this system:
			auto numCores = std::thread::hardware_concurrency() -1;//one for main

			// Calculate the actual number of worker threads we want:
			int numThreads = std::max(1u, numCores);
			for(int i = 0; i < numThreads; i++) {
				gManager.mWorkThreads.push_back(std::thread(&WorkerThread, i));
			}
		}
		void Shutdown() {
			//
			gManager.mQuit = true;
			gManager.m_CV.notify_all();
			for(int i = 0; i < gManager.mWorkThreads.size(); i++) {
				gManager.mWorkThreads[i].join();
			}
		}
	}; // namespace Worker

}; // namespace Job