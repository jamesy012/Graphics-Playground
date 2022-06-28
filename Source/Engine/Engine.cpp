#include "Engine.h"

#include "imgui.h"

#include "IGraphicsBase.h"
#include "Window.h"

#include "Job.h"
#include "PlatformDebug.h"

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

	WorkManager::Startup();
}

bool Engine::GameLoop() {
	ZoneScoped;

	{
		std::chrono::high_resolution_clock::time_point current = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> time_span				   = std::chrono::duration_cast<std::chrono::duration<double>>(current - mLastTime);

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

	GetWindow()->Update();
	WorkManager::ProcessMainThreadWork();
	return 0;
}

void Engine::Shutdown() {
	ASSERT(gEngine != nullptr);
	WorkManager::Shutdown();

	mGraphics->Destroy();
	mGraphics = nullptr;

	window.Destroy();

	gEngine = nullptr;
}

Window* Engine::GetWindow() const {
	return &window;
}

void Engine::ImGuiWindow() {
	if(ImGui::Begin("Engine")) {
		ImGui::Text("fps: %i\t(%i)", GetFPSAverage(), GetFPS());
		ImGui::Text("udt: %f utime: %f", mDeltaTimeUnscaled, mTimeSinceStartUnScaled);
		ImGui::Text(" dt: %f  time: %f", mDeltaTime, mTimeSinceStart);
		ImGui::DragFloat("Time Scale", &mTimeScale, 0.1f, 0.1f, 50.0f);
		ImGui::End();
	}
}