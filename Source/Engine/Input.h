#pragma once

#include <glm/glm.hpp>

//move to cpp?
//here to make it easier to use keys
#include <GLFW/glfw3.h>

class Input {
public:
	void StartUp();
	void AddWindow(GLFWwindow* aWindow);
	void Update();
	void Shutdown();

	void SetKeyState(int aKey, bool aState) {
		mKeyStates[aKey] = aState;
	}
	void SetMouseState(int aButton, bool aState) {
		mMouseStates[aButton] = aState;
	}
	void SetMousePos(glm::vec2 aNewPos) {
		mMouseDelta = aNewPos - mMousePos;
		mMousePos	= aNewPos;
	}

	constexpr bool IsKeyDown(int aKey) {
		return mKeyStates[aKey];
	}
	constexpr bool IsKeyUp(int aKey) {
		return !mKeyStates[aKey];
	}
	constexpr bool WasKeyPressed(int aKey) {
		return mKeyStates[aKey] && !mKeyStatesOld[aKey];
	}
	constexpr bool WasKeyReleased(int aKey) {
		return !mKeyStates[aKey] && mKeyStatesOld[aKey];
	}
	constexpr bool IsMouseButtonDown(int aButton) {
		return mMouseStates[aButton];
	}
	constexpr bool IsMouseButtonUp(int aButton) {
		return !mMouseStates[aButton];
	}
	constexpr bool WasMouseButtonPressed(int aButton) {
		return mMouseStates[aButton] && !mMouseStatesOld[aButton];
	}
	constexpr bool WasMouseButtonReleased(int aButton) {
		return !mMouseStates[aButton] && mMouseStatesOld[aButton];
	}

	constexpr glm::vec2 GetMousePos(){
		return mMousePos;
	}
	constexpr void GetMousePos(glm::vec2& aMousePos){
		aMousePos = mMousePos;
	}
	constexpr glm::vec2 GetMouseDelta(){
		return mMouseDelta;
	}
	constexpr void GetMouseDelta(glm::vec2& aMouseDelta){
		aMouseDelta = mMouseDelta;
	}

private:
	bool mKeyStates[400];
	bool mKeyStatesOld[400];
	bool mMouseStates[8];
	bool mMouseStatesOld[8];
	glm::vec2 mMousePos;
	glm::vec2 mMouseDelta;
};
extern Input* gInput;