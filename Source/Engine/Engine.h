#pragma once

#include <chrono>

class IGraphicsBase;

class Window;

//controls the opening of the game
//game loop
//which game states/levels are ticking
//threading?
//shutdown
class Engine {
public:
	void Startup(IGraphicsBase* aGraphics);
	bool GameLoop();
	void Shutdown();

	const double GetTimeSinceStart() const {
		return mTimeSinceStart;
	}
	const double GetTimeSinceStartUnscaled() const {
		return mTimeSinceStartUnScaled;
	}
	const double GetDeltaTime() const {
		return mDeltaTime;
	}
	const double GetDeltaTimeUnScaled() const {
		return mDeltaTimeUnscaled;
	}

	Window* GetWindow() const;

	void ImGuiWindow();

private:
	IGraphicsBase* mGraphics;

	//time at the last time we ran a game loop
	std::chrono::high_resolution_clock::time_point mLastTime;
	double mDeltaTime;
	double mDeltaTimeUnscaled;
	float mTimeScale			  = 1.0f;
	double mTimeSinceStart		  = 0.0f;
	double mTimeSinceStartUnScaled = 0.0f;
};

extern Engine* gEngine;