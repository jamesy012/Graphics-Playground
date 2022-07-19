#pragma once

#include <chrono>

class IGraphicsBase;

class Window;
class Camera;
class StateBase;

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

	StateBase* GetCurrentState() const {
		return mCurrentState;
	};
	void SetDesiredState(StateBase* aNewState);

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
	const int GetFPS() const {
		return mFPS[mFrameCount % NUM_FPS_COUNT];
	}
	const int GetFPSAverage() const {
		return mFPSTotal / NUM_FPS_COUNT;
	}

	Window* GetWindow() const;

	void SetMainCamera(Camera* aCamera) {
		mMainCamera = aCamera;
	}
	Camera* GetMainCamera() const{
		return mMainCamera;
	}

	bool IsMouseLocked() const;

private:
	void UpdateFramerate();
	void StateLogic();
	void ChangeStates();
	//imgui for engine class
	void ImGuiWindow();

	IGraphicsBase* mGraphics;

	StateBase* mCurrentState = nullptr;
	StateBase* mDesiredState = nullptr;

	//time at the last time we ran a game loop
	std::chrono::high_resolution_clock::time_point mLastTime;
	double mDeltaTime;
	double mDeltaTimeUnscaled;
	float mTimeScale = 1.0f;
	double mTimeSinceStart = 0.0f;
	double mTimeSinceStartUnScaled = 0.0f;
	int mFrameCount = 0;
	static const int NUM_FPS_COUNT = 128;
	int mFPS[NUM_FPS_COUNT] = {};
	int mFPSTotal = 0;

	Camera* mMainCamera = nullptr;
};

extern Engine* gEngine;