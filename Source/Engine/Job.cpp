#include "Job.h"

#include <mutex>
#include <thread>
#include <vector>
#include <deque>
#include <atomic>
#include <condition_variable>

#include "PlatformDebug.h"
#include "imgui.h"

struct WorkerManager {
public:
	//all thread
	std::vector<std::thread> mWorkThreads;
	//are we quitting and need to exit the threads
	bool mQuit = false;

	//work to do, removed when work started
	TracyLockable(std::mutex, mWorkAccesser);
	std::deque<Job::Work*> mWork;

	//finish jobs for the main thread to complete
	TracyLockable(std::mutex, mWorkMainAccesser);
	std::deque<Job::Work*> mWorkMain;

	//to sleep threads when no work in queue
	std::condition_variable_any m_CV;
} gManager;

//used for condition variable
bool WorkAvailable() {
	return gManager.mWork.size() != 0 || gManager.mQuit == true;
}

std::thread::id gMainThreadId;

Job::WorkState Job::WorkHandle::GetState() const {
	return mWorkRef ? mWorkRef->mWorkState : Job::WorkState::FINISHED;
}

void Job::WorkHandle::Reset() {
	if(mWorkRef != nullptr) {
		mWorkRef->mHandle = nullptr;
		mWorkRef = nullptr;
	}
	//delete this;
}

void Job::Work::DoWork(bool aOnlyFinish /*= false*/) {
	ZoneScoped;
	//work
	{
		if(mWorkPtr && aOnlyFinish == false) {
			ZoneScopedN("Work Ptr");
			mWorkPtr(mUserData);
			mWorkState = WorkState::MAIN_DONE;
		}
		if(mFinishPtr) {
			//should we finish on the main thread?
			if(mFinishOnMainThread && !Job::IsMainThread()) {
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

void Job::QueueWork(std::vector<Job::Work>& aWork, Job::WorkPriority aWorkPriority /* = WorkPriority::BOTTOM_OF_QUEUE*/) {
	ZoneScopedN("Queue Work Batched");
	const size_t numWork = aWork.size();
	std::string taskText = "Creating tasks - " + std::to_string(numWork);
	ZoneText(taskText.c_str(), taskText.size());

	std::vector<Job::Work*> work(numWork);
	for(int i = 0; i < numWork; i++) {
		work[i] = new Job::Work(aWork[i]);
		//pass through the new work to the handle if we have one
		if(work[i]->mHandle) {
			work[i]->mHandle->mWorkRef = work[i];
		}
	}

	//add work
	{
		std::unique_lock lock(gManager.mWorkAccesser);
		switch(aWorkPriority) {
			case WorkPriority::BOTTOM_OF_QUEUE:
				gManager.mWork.insert(gManager.mWork.end(), work.begin(), work.end());
				break;
			case WorkPriority::TOP_OF_QUEUE:
				gManager.mWork.insert(gManager.mWork.begin(), work.begin(), work.end());
				break;
			default:
				ASSERT(false);
		}
		gManager.m_CV.notify_all();
		lock.unlock();
	}
}

void Job::QueueWork(Job::Work& aWork, Job::WorkPriority aWorkPriority /* = WorkPriority::BOTTOM_OF_QUEUE*/) {
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

std::vector<Job::WorkHandle*> Job::QueueWorkHandle(std::vector<Job::Work>& aWork, WorkPriority aWorkPriority /* = WorkPriority::BOTTOM_OF_QUEUE*/) {
	const size_t numWork = aWork.size();
	std::vector<Job::WorkHandle*> handles(numWork);
	for(int i = 0; i < numWork; i++) {
		handles[i]		 = new Job::WorkHandle();
		aWork[i].mHandle = handles[i];
	}
	QueueWork(aWork, aWorkPriority);
	return handles;
}

Job::WorkHandle* Job::QueueWorkHandle(Job::Work& aWork, WorkPriority aWorkPriority /* = WorkPriority::BOTTOM_OF_QUEUE*/) {
	aWork.mHandle = new Job::WorkHandle();
	QueueWork(aWork, aWorkPriority);
	return aWork.mHandle;
}

bool Job::WaitForWork(const Job::WorkHandle* aHandle) {
	ZoneScoped;
	if(aHandle == nullptr) {
		return false;
	}
	if(aHandle->mIsDone) {
		return true;
	}
	//modifying arrays, lock thread
	std::unique_lock lock(gManager.mWorkAccesser);
	Work* work = aHandle->mWorkRef;
	switch(work->mWorkState) {
		case WorkState::FINISHED: //task already finished, no waiting
			lock.unlock();
			return true;
		case WorkState::QUEUED: //task not started, wait by doing the task in this thread
		{
			auto result = std::find(gManager.mWorkMain.begin(), gManager.mWorkMain.end(), work);
			gManager.mWorkMain.erase(result);
			lock.unlock();
			if(Job::IsMainThread() && work->mFinishOnMainThread) {
				ASSERT(false); //on main thread, needing finish work on main thread?
				return false;
			} else {
				work->DoWork();
			}
		}
			return true;
		default: //work started, wait for it to finish
			lock.unlock();
			if(Job::IsMainThread() && work->mFinishOnMainThread) {
				ASSERT(false); //on main thread, needing finish work on main thread?
				return false;
			}
			//todo revisit Work wait
			do {
			} while(!aHandle->mIsDone); //keep checking
			return true;
	}
	lock.unlock();
	return true;
}

bool Job::IsMainThread() {
	return std::this_thread::get_id() == gMainThreadId;
}

int Job::GetWorkRemaining() {
	ZoneScoped;
	std::unique_lock lock(gManager.mWorkAccesser);
	std::unique_lock lock2(gManager.mWorkMainAccesser);
	int result = gManager.mWork.size() + gManager.mWorkMain.size();
	lock.unlock();
	lock2.unlock();
	return result;
}

void Job::SpinSleep(float aLength) {
	ZoneScoped;
	std::string lengthText = "Sleep for " + std::to_string(aLength * 1000) + "ms";
	ZoneText(lengthText.c_str(), lengthText.size());

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	double ms										  = 0;
	while(ms < aLength) {
		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> time_span			  = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
		ms												  = time_span.count();
	}
}

//Manager/Async Thread
void WorkerThread(int aThreadIndex) {
#if defined(TRACY_ENABLE)
	std::string text = "Worker thread: " + std::to_string(aThreadIndex);
	tracy::SetThreadName(text.c_str());
#endif
	auto hash = std::hash<std::thread::id>();
	srand(hash(std::this_thread::get_id()));
	while(true) {
		Job::Work* work;
		//get work
		{
			std::unique_lock lock(gManager.mWorkAccesser);
			gManager.m_CV.wait(lock, WorkAvailable);
			if(gManager.mQuit) {
				break;
			}
			work = gManager.mWork.front();
			gManager.mWork.pop_front();
			work->mWorkState = Job::WorkState::STARTED;
			lock.unlock();
		}

		work->DoWork();
		work = nullptr;
	}
	LOGGER::Formated("Worker thread {} Closing\n", aThreadIndex);
}

void WorkManager::Startup() {
	gMainThreadId = std::this_thread::get_id();

	// Retrieve the number of hardware threads in this system:
	unsigned int numCores = std::thread::hardware_concurrency() - 1; //one for main

	LOGGER::Formated("Starting Job system with {} threads\n", numCores);

	// Calculate the actual number of worker threads we want:
	int numThreads = std::max(1u, numCores);
	for(int i = 0; i < numThreads; i++) {
		gManager.mWorkThreads.push_back(std::thread(&WorkerThread, i));
	}
}
double gMsTimeTaken;
int gWorkDone;
void WorkManager::ProcessMainThreadWork() {
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
		Job::Work* work;
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

void WorkManager::Shutdown() {
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

void WorkManager::ImGuiTesting() {
	ImGui::Begin("Job Test");
	static std::vector<Job::WorkHandle*> mWaitingHandles;
	if(ImGui::Button("Add Job that queues jobs")) {
		Job::Work work;
		work.mWorkPtr = [](void*) {
			Job::Work work;
			work.mWorkPtr = [](void*) {
				Job::SpinSleep(50 / 1000.0f);
			};
			for(int i = 0; i < 100; i++) {
				Job::QueueWork(work);
			}
		};
		Job::QueueWork(work, Job::WorkPriority::TOP_OF_QUEUE);
	};
	if(ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Does not add to the waiting handles list due to multithreading");
	}
	static Job::WorkHandle* handleWaitTest = nullptr;
	if(ImGui::Button("Add Job Wait Test") && handleWaitTest == nullptr) {
		Job::Work work;
		work.mWorkPtr = [](void*) {
			Job::Work work;
			work.mWorkPtr = [](void*) {
				Job::SpinSleep(5000 / 1000.0f);
			};
			handleWaitTest = Job::QueueWorkHandle(work);
		};
		Job::QueueWork(work, Job::WorkPriority::TOP_OF_QUEUE);
	}
	if(handleWaitTest && handleWaitTest->GetState() != Job::WorkState::QUEUED) {
		ImGui::SameLine();
		if(ImGui::Button("Main thread wait for job")) {
			Job::WaitForWork(handleWaitTest);
		}
		if(handleWaitTest->GetState() == Job::WorkState::FINISHED) {
			handleWaitTest->Reset();
			handleWaitTest = nullptr;
		}
	}
	if(ImGui::Button("Add Job main short sleep x100")) {
		for(int i = 0; i < 100; i++) {
			Job::Work work;
			work.mFinishOnMainThread = true;
			work.mFinishPtr			 = [](void*) {
				 ZoneScoped;
				 int length = 1 + (rand() % 50);

				 std::string text = std::to_string(length);
				 ZoneText(text.c_str(), text.size());

				 float msLength = length / 1000.0f;
				 Job::SpinSleep(msLength);
				 LOGGER::Formated("I have finished {}\n", Job::IsMainThread());
			};
			mWaitingHandles.push_back(Job::QueueWorkHandle(work));
		}
	}
	if(ImGui::Button("Add Batch Job main short sleep x100")) {
		std::vector<Job::Work> workArray(100);
		for(int i = 0; i < workArray.size(); i++) {
			Job::Work& work			 = workArray[i];
			work.mFinishOnMainThread = true;
			work.mFinishPtr			 = [](void*) {
				 ZoneScoped;
				 int length = 1 + (rand() % 50);

				 std::string text = std::to_string(length);
				 ZoneText(text.c_str(), text.size());

				 float msLength = length / 1000.0f;
				 Job::SpinSleep(msLength);
				 LOGGER::Formated("I have finished {}\n", Job::IsMainThread());
			};
		}
		std::vector<Job::WorkHandle*> workHandle = Job::QueueWorkHandle(workArray);
		mWaitingHandles.insert(mWaitingHandles.end(), workHandle.begin(), workHandle.end());
	}
	if(ImGui::Button("Add Job sleep x20")) {
		for(int i = 0; i < 20; i++) {
			Job::Work work;
			work.mWorkPtr = [](void*) {
				ZoneScoped;
				int length = 100 + (rand() % 1000);

				std::string text = std::to_string(length);
				ZoneText(text.c_str(), text.size());

				//_sleep(length);

				float msLength = length / 1000.0f;
				Job::SpinSleep(msLength);
			};
			work.mFinishPtr = [](void*) {
				LOGGER::Formated("I have slept enough\n");
			};
			mWaitingHandles.push_back(Job::QueueWorkHandle(work));
		}
	}
	ImGui::Text("Async Work Remaining: %i", Job::GetWorkRemaining());
	if(mWaitingHandles.size()) {
		ImGui::Text("Main: Did %i work %f", WorkManager::GetWorkCompleted(), WorkManager::GetWorkLength());
		ImGui::Text("Waiting on %i sleeps", (int)mWaitingHandles.size());
		for(int i = 0; i < mWaitingHandles.size(); i++) {
			Job::WorkHandle* handle = mWaitingHandles[i];
			if(Job::IsDone(handle)) {
				mWaitingHandles.erase(mWaitingHandles.begin() + i);
				i--;
				handle->Reset();
			}
		}
		if(mWaitingHandles.size() == 0) {
			LOGGER::Formated("SLEEP Work finished\n");
		}
	}
	ImGui::End();
}

int WorkManager::GetWorkCompleted() {
	return gWorkDone;
}
double WorkManager::GetWorkLength() {
	return gMsTimeTaken;
}
