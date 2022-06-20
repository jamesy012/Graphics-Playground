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

	//finish jobs for the main thread to complete
	TracyLockable(std ::mutex, mWorkMainAccesser);
	std::deque<Job::Work> mWorkMain;

	//to sleep threads when no work in queue
	std::condition_variable_any m_CV;

	//replacement if mWork doesnt work? probs needs to be larger than char
	//std::atomic_char mWorkCount;

} gManager;
bool WorkAvailable() {
	//return gManager.mWorkCount != 0 || gManager.mQuit == true;
	return gManager.mWork.size() != 0 || gManager.mQuit == true;
}

std::thread::id gMainThreadId;

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

	Workhandle QueueWork(WorkFunction aWork) {
		Work work	 = {};
		work.workPtr = aWork;
		return QueueWork(work);
	}
	Workhandle QueueWork() {
		Work work = {};
		return QueueWork(work);
	}

	bool IsFinished(Workhandle aHandle) {
		std::unique_lock lock(gManager.mWorkHandleAccesser);
		const auto result	= std::find(gManager.mWorkHandles.begin(), gManager.mWorkHandles.end(), aHandle);
		const bool finished = result == gManager.mWorkHandles.end();
		lock.unlock();
		return finished;
	}

	bool IsMainThread() {
		return std::this_thread::get_id() == gMainThreadId;
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
					}
					if(work.finishPtr) {
						//todo - add work to another queue for a call later on the main thread
						if(work.finishOnMainThread) {
							std::unique_lock lock(gManager.mWorkMainAccesser);
							gManager.mWorkMain.push_back(work);
							lock.unlock();
							//dont do anything else here, work is done
							continue;
						} else {
							ZoneScopedN("Finish Ptr");
							work.finishPtr();
						}
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
			gMainThreadId = std::this_thread::get_id();

			// Retrieve the number of hardware threads in this system:
			unsigned int numCores = std::thread::hardware_concurrency() - 1; //one for main
			LOGGER::Formated("Starting Job system with {} threads", numCores);
			// Calculate the actual number of worker threads we want:
			int numThreads = std::max(1u, numCores);
			for(int i = 0; i < numThreads; i++) {
				gManager.mWorkThreads.push_back(std::thread(&WorkerThread, i));
			}
		}
		double gMsTimeTaken;
		int gWorkDone;
		void ProcessMainThreadWork() {
			ZoneScoped;
			gWorkDone										  = 0;
			const double maxLength							  = 0.05f;
			std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
			gMsTimeTaken									  = 0;
			while(true) {
				{ //limit time
					std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> time_span			  = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
					gMsTimeTaken									  = time_span.count();
					if(gMsTimeTaken > maxLength) {
						return;
					}
				}
				ZoneScoped;
				Work work;
				{ //get work
					ZoneScoped;
					std::unique_lock lock(gManager.mWorkMainAccesser);
					if(gManager.mWorkMain.size() == 0) {
						//no work
						lock.unlock();
						return;
					}
					work = gManager.mWorkMain.front();
					gManager.mWorkMain.pop_front();
					lock.unlock();
				}
				{ //do work
					ZoneScoped;
					work.finishPtr();
				}
				{ //finish work
					ZoneScoped;
					std::unique_lock lock(gManager.mWorkHandleAccesser);
					auto result = std::find(gManager.mWorkHandles.begin(), gManager.mWorkHandles.end(), work.handle);
					gManager.mWorkHandles.erase(result);
					lock.unlock();
					gWorkDone++;
				}
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
			LOGGER::Formated("We left async:{} main:{} work unfinished...\n", gManager.mWork.size(), gManager.mWorkMain.size());
		}

		int GetWorkCompleted(){
			return gWorkDone;
		}
		double GetWorkLength(){
			return gMsTimeTaken;
		}
	}; // namespace Worker

}; // namespace Job