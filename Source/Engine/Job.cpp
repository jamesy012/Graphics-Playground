#include "Job.h"

#include <mutex>
#include <thread>
#include <vector>
#include <deque>
#include <atomic>
#include <condition_variable>

#include "PlatformDebug.h"

#include <future>

struct WorkerManager {
public:
	//all thread
	std::vector<std::thread> mWorkThreads;
	//are we quitting and need to exit the threads
	bool mQuit = false;

	//work to do, removed when work started
	TracyLockable(std ::mutex, mWorkAccesser);
	std::deque<Job::Work*> mWork;

	//finish jobs for the main thread to complete
	TracyLockable(std ::mutex, mWorkMainAccesser);
	std::deque<Job::Work*> mWorkMain;

	//to sleep threads when no work in queue
	std::condition_variable_any m_CV;
} gManager;
bool WorkAvailable() {
	return gManager.mWork.size() != 0 || gManager.mQuit == true;
}

std::thread::id gMainThreadId;

namespace Job {

	void Job::Work::DoWork(bool aOnlyFinish /*= false*/) {
		ZoneScoped;
		//work
		{
			if(mWorkPtr && aOnlyFinish == false) {
				ZoneScoped;
				ZoneScopedN("Work Ptr");
				mWorkPtr(mUserData);
				mWorkState = WorkState::MAIN_DONE;
			}
			if(mFinishPtr) {
				ZoneScoped;
				//should we finish on the main thread?
				if(mFinishOnMainThread && !IsMainThread()) {
					ZoneScopedN("Queue to main thread");
					mWorkState = WorkState::FINISHING_MAIN;
					std::unique_lock lock(gManager.mWorkMainAccesser);
					gManager.mWorkMain.push_back(this);
					lock.unlock();
					//dont do anything else here, work is done
					return;
				} else { //otherwise just continue on
					ZoneScopedN("Finish Ptr");
					mWorkState = WorkState::FINISHING;
					mFinishPtr(mUserData);
					mWorkState = WorkState::FINISHED;
				}
			}
		}
		//finish work
		{
			//std::unique_lock lock(gManager.mWorkHandleAccesser);
			////lock.lock();
			//auto result = std::find(gManager.mWorkHandles.begin(), gManager.mWorkHandles.end(), handle);
			//gManager.mWorkHandles.erase(result);
			//lock.unlock();
			if(mHandle != nullptr) {
				mHandle->mIsDone  = true;
				mHandle->mWorkRef = nullptr;
			}
			delete this;
		}
	}

	void QueueWork(Work& aWork, WorkPriority aWorkPriority /* = WorkPriority::BOTTOM_OF_QUEUE*/) {
		ZoneScoped;
		Work* work = new Work(aWork);
		//pass through the new work to the handle if we have one
		if(work->mHandle) {
			work->mHandle->mWorkRef = work;
		}

		//add work
		{
			std::unique_lock lock(gManager.mWorkAccesser);
			switch(aWorkPriority) {
				case WorkPriority::BOTTOM_OF_QUEUE:
					gManager.mWork.push_back(work);
					break;
				case WorkPriority::TOP_OF_QUEUE:
					gManager.mWork.push_front(work);
					break;
				default:
					ASSERT(false);
			}
			gManager.m_CV.notify_one();
			lock.unlock();
		}
	}

	WorkHandle* QueueWorkHandle(Work& aWork, WorkPriority aWorkPriority /* = WorkPriority::BOTTOM_OF_QUEUE*/) {
		aWork.mHandle = new WorkHandle();
		QueueWork(aWork, aWorkPriority);
		return aWork.mHandle;
	}

	bool IsFinished(WorkHandle* aHandle) {
		return aHandle->mIsDone;
	}

	bool IsMainThread() {
		return std::this_thread::get_id() == gMainThreadId;
	}

	int GetWorkRemaining() {
		std::unique_lock<std::mutex> lock(gManager.mWorkAccesser);
		std::unique_lock<std::mutex> lock2(gManager.mWorkMainAccesser);
		int result = gManager.mWork.size() + gManager.mWorkMain.size();
		lock.unlock();
		lock2.unlock();
		return result;
	}

	void SpinSleep(float aLength) {
		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		double ms										  = 0;
		while(ms < aLength) {
			std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> time_span			  = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
			ms												  = time_span.count();
		}
	}

	namespace Worker {

		void WorkerThread(int aThreadIndex) {
#if defined(TRACY_ENABLE)
			std::string text = "Worker thread: " + std::to_string(aThreadIndex);
			tracy::SetThreadName(text.c_str());
#endif
			auto hash = std::hash<std::thread::id>();
			srand(hash(std::this_thread::get_id()));
			while(true) {
				Work* work;
				//get work
				{
					std::unique_lock lock(gManager.mWorkAccesser);
					gManager.m_CV.wait(lock, WorkAvailable);
					if(gManager.mQuit) {
						break;
					}
					work = gManager.mWork.front();
					gManager.mWork.pop_front();
					work->mWorkState = WorkState::STARTED;
					lock.unlock();
				}

				work->DoWork();
				work = nullptr;
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
				Work* work;
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
				//do work without calling the main work part
				//since that's already been done
				work->DoWork(true);
				gWorkDone++;
			}
		}

		void Shutdown() {
			//
			gManager.mQuit = true;
			gManager.m_CV.notify_all();
			for(int i = 0; i < gManager.mWorkThreads.size(); i++) {
				gManager.mWorkThreads[i].join();
			}
			//no need to lock anymore, all threading is done
			if(gManager.mWork.size() + gManager.mWorkMain.size()) {
				//should we finish this work here or just forget about it?
				LOGGER::Formated("We left async:{} main:{} work unfinished...\n", gManager.mWork.size(), gManager.mWorkMain.size());
			}
		}

		int GetWorkCompleted() {
			return gWorkDone;
		}
		double GetWorkLength() {
			return gMsTimeTaken;
		}
	}; // namespace Worker

}; // namespace Job