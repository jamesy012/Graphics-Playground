#include "Window.h"

#include <vulkan/vulkan.h>

#if PLATFORM_WINDOWS
#	define VK_USE_PLATFORM_WIN32_KHR
#	include <GLFW/glfw3.h>
#	define GLFW_EXPOSE_NATIVE_WIN32
#	include <GLFW/glfw3native.h>
#elif PLATFORM_APPLE
#	include <GLFW/glfw3.h>
#endif

#include "PlatformDebug.h"
#include "AllocationCallbacks.h"

#include "Engine/Input.h"

extern VkInstance gVkInstance;

void Window::Create(const int aWidth, const int aHeight, const char* aTitle) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	mWindow = glfwCreateWindow(aWidth, aHeight, aTitle, nullptr, nullptr);
}

void Window::Destroy() {
	glfwDestroyWindow(mWindow);
	mWindow = nullptr;
}

//vulkan stuff should call into Graphics folder not Engine?
void* Window::CreateSurface() {
	ASSERT(gVkInstance != VK_NULL_HANDLE);
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	const VkResult result = glfwCreateWindowSurface(gVkInstance, mWindow, GetAllocationCallback(), &surface);
	if(result != VK_SUCCESS) {
		ASSERT(false); // Failed to create surface
	}
	mSurface = surface;
	return mSurface;
}

void Window::DestroySurface() {
	ASSERT(gVkInstance != VK_NULL_HANDLE);
	ASSERT(mSurface != VK_NULL_HANDLE);
	vkDestroySurfaceKHR(gVkInstance, (VkSurfaceKHR)mSurface, GetAllocationCallback());
}

bool Window::ShouldClose() {
	return glfwWindowShouldClose(mWindow);
}

void Window::WaitEvents() {
	return glfwWaitEvents();
}

void Window::Update() {
	ZoneScoped;
	glfwPollEvents();

	if(mLocked) {
		//if(HasFocus()) {
		//	int width, height;
		//	GetSize(&width, &height);
		//	glfwSetCursorPos(mWindow, width / 2, height / 2);
		//}
		if(gInput->WasKeyPressed(GLFW_KEY_ESCAPE)) {
			mLocked = false;
			glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	} else {
		if(HasFocus()) {
			if(gInput->WasMouseButtonPressed(GLFW_MOUSE_BUTTON_1)) {
				mLocked = true;
				glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			if(gInput->WasKeyPressed(GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(mWindow, true);
			}
		}
	}
}

const char** Window::GetGLFWVulkanExtentensions(uint32_t* aCount) {
	return glfwGetRequiredInstanceExtensions(aCount);
}

void Window::GetSize(int* aWidth, int* aHeight) const {
	glfwGetWindowSize(mWindow, aWidth, aHeight);
}

bool Window::HasFocus() const {
	return glfwGetWindowAttrib(mWindow, GLFW_FOCUSED) != 0;
}
