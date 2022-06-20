#include "Engine.h"

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

	Job::Worker::Startup();
}

bool Engine::GameLoop() {
	ZoneScoped;
	GetWindow()->Update();
	Job::Worker::ProcessMainThreadWork();
	return 0;
}

void Engine::Shutdown() {
	ASSERT(gEngine != nullptr);
	Job::Worker::Shutdown();

	mGraphics->Destroy();
	mGraphics = nullptr;

	window.Destroy();

	gEngine = nullptr;
}

Window* Engine::GetWindow() const {
	return &window;
}