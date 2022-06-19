#include "Job.h"

#include <mutex>
#include <thread>
#include <vector>
#include <deque>
#include <atomic>
#include <condition_variable>

#include "PlatformDebug.h"

struct WorkerManager {
public:
	//all thread
	std::vector<std::thread> mWorkThreads;
	//are we quitting and need to exit the threads
	bool mQuit = false;

	//work to do, removed when work started
	TracyLockable(std ::mutex, mWorkAccesser);
	std::deque<Job::Work> mWork;

	//active jobs, removed when work finished
	TracyLockable(std ::mutex, mWorkHandleAccesser);
	std::deque<Job::Workhandle> mWorkHandles;

	//to sleep threads when no work in queue
	std::condition_variable_any m_CV;

	//replacement if mWork doesnt work? probs needs to be larger than char
	//std::atomic_char mWorkCount;

} gManager;
bool WorkAvailable() {
	//return gManager.mWorkCount != 0 || gManager.mQuit == true;
	return gManager.mWork.size() != 0 || gManager.mQuit == true;
}
namespace Job {

	std::atomic<Workhandle> gWorkHandleCounter;

	Workhandle QueueWork(Work& aWork) {
		ZoneScoped;
		aWork.handle = gWorkHandleCounter++;
		//add handle
		{
			std::unique_lock lock(gManager.mWorkHandleAccesser);
			gManager.mWorkHandles.push_front(aWork.handle);
			lock.unlock();
		}
		//add work
		{
			std::unique_lock lock(gManager.mWorkAccesser);
			gManager.mWork.push_back(aWork);
			//gManager.mWorkCount++;
			gManager.m_CV.notify_one();
			lock.unlock();
		}
		LOGGER::Formated("Work Added\n");
		return aWork.handle;
	}

	Workhandle QueueWork(int aWork) {
		Work work  = {};
		work.value = aWork;
		return QueueWork(work);
	}
	Workhandle QueueWork(WorkFunction aWork) {
		Work work	 = {};
		work.workPtr = aWork;
		return QueueWork(work);
	}

	bool IsFinished(Workhandle aHandle) {
		std::unique_lock lock(gManager.mWorkHandleAccesser);
		const auto result	= std::find(gManager.mWorkHandles.begin(), gManager.mWorkHandles.end(), aHandle);
		const bool finished = result == gManager.mWorkHandles.end();
		lock.unlock();
		return finished;
	}

	namespace Worker {

		void WorkerThread(int aThreadIndex) {
#if defined(TRACY_ENABLE)
			std::string text = "Worker thread: " + std::to_string(aThreadIndex);
			tracy::SetThreadName(text.c_str());
#endif

			LOGGER::Formated("Work setup\n");
			auto hash = std::hash<std::thread::id>();
			srand(hash(std::this_thread::get_id()));
			while(true) {
				Work work;
				//get work
				{
					std::unique_lock lock(gManager.mWorkAccesser);
					gManager.m_CV.wait(lock, WorkAvailable);
					if(gManager.mQuit) {
						break;
					}
					work = gManager.mWork.front();
					gManager.mWork.pop_back();
					lock.unlock();
				}
				//work
				{
					ZoneScoped;
					LOGGER::Formated("Work Started {}\n", aThreadIndex);
					if(work.workPtr) {
						ZoneScopedN("Work Ptr");
						work.workPtr();
					} else {
						LOGGER::Formated("\tWork value {}\n", (int)work.value);
					}
					if(work.finishPtr) {
						//todo - add work to another queue for a call later on the main thread
						ASSERT(!work.finishOnMainThread);
						ZoneScopedN("Finish Ptr");
						work.finishPtr();
					}
					LOGGER::Formated("Work Finished {}\n", aThreadIndex);
				}
				//finish work
				{
					std::unique_lock lock(gManager.mWorkHandleAccesser);
					//lock.lock();
					auto result = std::find(gManager.mWorkHandles.begin(), gManager.mWorkHandles.end(), work.handle);
					gManager.mWorkHandles.erase(result);
					lock.unlock();
				}
			}
			LOGGER::Formated("Worker thread {} Closing\n", aThreadIndex);
		}

		void Startup() {

			// Retrieve the number of hardware threads in this system:
			unsigned int numCores = std::thread::hardware_concurrency() - 1; //one for main
			LOGGER::Formated("Starting Job system with {} threads", numCores);
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
			//should we finish this work here or just forget about it?
			LOGGER::Formated("We left {} work unfinished...\n", gManager.mWork.size());
		}
	}; // namespace Worker

}; // namespace Job