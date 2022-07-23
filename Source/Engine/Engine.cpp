#include "Engine.h"

#include "imgui.h"

#include "PlatformDebug.h"

#include "IGraphicsBase.h"
#include "Physics.h"
#include "Job.h"
#include "Window.h"
#include "Input.h"
#include "Camera/Camera.h"
#include "StateBase.h"

Window window;
Engine* gEngine = nullptr;

void Engine::Startup(IGraphicsBase* aGraphics) {
	ASSERT(gEngine == nullptr);
	gEngine = this;
	LOGGER::Log("Starting Engine\n");

	window.Create(720, 720, "Graphics Playground");

	mGraphics = aGraphics;

	mGraphics->Startup();
	mGraphics->AddWindow(&window);
	mGraphics->Initalize();
	LOGGER::Log("Graphics Initalized\n");

	Input* input = new Input();
	input->StartUp();
	input->AddWindow(window.GetWindow());
	LOGGER::Log("Input Initalized\n");

	mPhysics = new Physics();
	mPhysics->Startup();

	WorkManager::Startup();
}

bool Engine::GameLoop() {

	while(!gEngine->GetWindow()->ShouldClose()) {
		ZoneScoped;
		UpdateFramerate();

		gInput->Update();
		GetWindow()->Update();
		WorkManager::ProcessMainThreadWork();

		mGraphics->StartNewFrame();

		gEngine->ImGuiWindow();
		WorkManager::ImGuiTesting();

		if(mMainCamera) {
			mMainCamera->Update();
		}

		//graphics mutex locked in this zone
		{
			mGraphics->StartGraphicsFrame();

			StateLogic();

			mGraphics->EndFrame();
		}

		ChangeStates();
	}

	//game exiting lets clean up the active state by running one more loop
	SetDesiredState(nullptr);
	ChangeStates();

	return 0;
}

void Engine::Shutdown() {
	ASSERT(gEngine != nullptr);
	WorkManager::Shutdown();

	mPhysics->Shutdown();
	delete mPhysics;
	mPhysics = nullptr;

	gInput->Shutdown();
	delete gInput;
	gInput = nullptr;

	mGraphics->Destroy();
	mGraphics = nullptr;

	window.Destroy();

	gEngine = nullptr;
}

void Engine::SetDesiredState(StateBase* aNewState) {
	ZoneScoped;
	if(mCurrentState == aNewState) {
		return;
	}
	//two new states pushed this frame?
	//could possibly finish/destroy the other requested state
	ASSERT(mDesiredState == mCurrentState);
	//todo async, only change when finished
	mDesiredState = aNewState;
	//if(mCurrentState) {
	//	mCurrentState->Finish();
	//}
	if(mDesiredState) {
		mDesiredState->Initalize();
	}
}

Window* Engine::GetWindow() const {
	return &window;
}

bool Engine::IsMouseLocked() const {
	return window.IsLocked();
}

void Engine::UpdateFramerate() {
	std::chrono::high_resolution_clock::time_point current = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(current - mLastTime);

	mLastTime = current;

	mDeltaTimeUnscaled = time_span.count();
	if(mDeltaTimeUnscaled > 1) {
		mDeltaTimeUnscaled = 0.01f;
	}
	mDeltaTime = mDeltaTimeUnscaled * mTimeScale;
	mTimeSinceStartUnScaled += mDeltaTimeUnscaled;
	mTimeSinceStart += mDeltaTime;

	mFrameCount++;
	mFPSTotal -= mFPS[mFrameCount % NUM_FPS_COUNT];
	mFPS[mFrameCount % NUM_FPS_COUNT] = 1 / mDeltaTimeUnscaled;
	mFPSTotal += mFPS[mFrameCount % NUM_FPS_COUNT];
}

void Engine::StateLogic() {
	ZoneScopedN("State Update");
	if(mCurrentState) {
		{
			ZoneScopedN("State ImGui");
			mCurrentState->ImGuiRender();
		}
		{
			ZoneScopedN("State Update");
			mCurrentState->Update();
		}
		{
			ZoneScopedN("State Render");
			mCurrentState->Render();
		}
	}
}

void Engine::ChangeStates() {
	ZoneScoped;
	//should this happen before the update?
	if(mDesiredState != mCurrentState) {
		if(mCurrentState) {
			mCurrentState->Finish(); //should possibly be called when the desired state is set?
			mCurrentState->Destroy();
		}
		mCurrentState = mDesiredState;
		if(mDesiredState) {
			mDesiredState->StartUp();
		}
	}
}

void Engine::ImGuiWindow() {
	if(ImGui::Begin("Engine")) {
		ImGui::Text("fps: %i\t(%i)\t(%f)", GetFPSAverage(), GetFPS(), ImGui::GetIO().Framerate);
		ImGui::Text("udt: %f utime: %f", mDeltaTimeUnscaled, mTimeSinceStartUnScaled);
		ImGui::Text(" dt: %f  time: %f", mDeltaTime, mTimeSinceStart);
		ImGui::DragFloat("Time Scale", &mTimeScale, 0.1f, 0.1f, 50.0f);
		ImGui::End();
	}
}