#include "Input.h"

#include "PlatformDebug.h"

#if defined(ENABLE_IMGUI)
#	include "imgui_impl_glfw.h"
#endif

Input* gInput = nullptr;

void GLFWmousebuttonCallback(GLFWwindow* window, int button, int action, int mods) {
	if(action == GLFW_PRESS || action == GLFW_RELEASE) {
		gInput->SetMouseState(button, action == GLFW_PRESS);
	}
#if defined(ENABLE_IMGUI)
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

void GLFWcursorposCallback(GLFWwindow* window, double xpos, double ypos) {
	glm::vec2 mousePos(xpos, ypos);
	gInput->SetMousePos(mousePos);
#if defined(ENABLE_IMGUI)
	ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
#endif
}

void GLFWkeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(action == GLFW_PRESS || action == GLFW_RELEASE) {
		gInput->SetKeyState(key, action == GLFW_PRESS);
	}
#if defined(ENABLE_IMGUI)
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
#endif
}

void Input::StartUp() {
	ASSERT(gInput == nullptr);
	gInput = this;

	memset(mKeyStates, 0, sizeof(mKeyStates));
	memset(mKeyStatesOld, 0, sizeof(mKeyStates));
	memset(mMouseStates, 0, sizeof(mMouseStates));
	memset(mMouseStatesOld, 0, sizeof(mMouseStates));
}

void Input::AddWindow(GLFWwindow* aWindow) {
	glfwSetMouseButtonCallback(aWindow, GLFWmousebuttonCallback);
	glfwSetCursorPosCallback(aWindow, GLFWcursorposCallback);
	glfwSetKeyCallback(aWindow, GLFWkeyCallback);
}

void Input::Update() {
	memcpy(mKeyStatesOld, mKeyStates, sizeof(mKeyStates));
	memcpy(mMouseStatesOld, mMouseStates, sizeof(mMouseStates));
}

void Input::Shutdown() {
	ASSERT(gInput != nullptr);
	gInput = nullptr;
}