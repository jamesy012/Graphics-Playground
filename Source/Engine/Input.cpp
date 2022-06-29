#include "Input.h"

#include "PlatformDebug.h"

#if defined(ENABLE_IMGUI)
#	include "imgui.h"
#	include "imgui_impl_glfw.h"
#endif

Input* gInput = nullptr;

void GLFWmousebuttonCallback(GLFWwindow* window, int button, int action, int mods) {
#if defined(ENABLE_IMGUI)
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	ImGuiIO& io = ImGui::GetIO();
	if(io.WantCaptureMouse) {
		gInput->ResetMouseButtons();
		return;
	}
#endif
	if(action == GLFW_PRESS || action == GLFW_RELEASE) {
		gInput->SetMouseState(button, action == GLFW_PRESS);
	}
}

void GLFWcursorposCallback(GLFWwindow* window, double xpos, double ypos) {
#if defined(ENABLE_IMGUI)
	ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
#endif
	glm::vec2 mousePos(xpos, ypos);
	gInput->SetMousePos(mousePos);
}

void GLFWkeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
#if defined(ENABLE_IMGUI)
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	ImGuiIO& io = ImGui::GetIO();
	if(io.WantCaptureKeyboard) {
		gInput->ResetKeys();
		return;
	}
#endif
	if(action == GLFW_PRESS || action == GLFW_RELEASE) {
		gInput->SetKeyState(key, action == GLFW_PRESS);
	}
}

void Input::StartUp() {
	ASSERT(gInput == nullptr);
	gInput = this;

	ResetKeys();
	ResetMouseButtons();
}

void Input::AddWindow(GLFWwindow* aWindow) {
	glfwSetMouseButtonCallback(aWindow, GLFWmousebuttonCallback);
	glfwSetCursorPosCallback(aWindow, GLFWcursorposCallback);
	glfwSetKeyCallback(aWindow, GLFWkeyCallback);
}

void Input::Update() {
	memcpy(mKeyStatesOld, mKeyStates, sizeof(mKeyStates));
	memcpy(mMouseStatesOld, mMouseStates, sizeof(mMouseStates));
	mMouseDelta = mMousePos - mMousePosOld;
	mMousePosOld = mMousePos;
}

void Input::Shutdown() {
	ASSERT(gInput != nullptr);
	gInput = nullptr;
}

void Input::ResetKeys() {
	memset(mKeyStates, 0, sizeof(mKeyStates));
	memset(mKeyStatesOld, 0, sizeof(mKeyStates));
}
void Input::ResetMouseButtons() {
	memset(mMouseStates, 0, sizeof(mMouseStates));
	memset(mMouseStatesOld, 0, sizeof(mMouseStates));
	mMouseDelta = mMousePos = mMousePosOld = glm::vec2(0);
}