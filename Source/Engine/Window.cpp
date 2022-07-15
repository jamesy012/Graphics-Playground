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

	glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
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

	if(!mLocked && HasFocus()) {
		if(gInput->WasKeyPressed(GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(mWindow, true);
		}
	}
}

const char** Window::GetGLFWVulkanExtentensions(uint32_t* aCount) {
	return glfwGetRequiredInstanceExtensions(aCount);
}

void Window::GetSize(int* aWidth, int* aHeight) const {
	glfwGetWindowSize(mWindow, aWidth, aHeight);
}
void Window::GetFramebufferSize(int* aWidth, int* aHeight) const {
	glfwGetFramebufferSize(mWindow, aWidth, aHeight);
}

bool Window::HasFocus() const {
	return glfwGetWindowAttrib(mWindow, GLFW_FOCUSED) != 0;
}

void Window::SetLock(const bool aShouldLock) {
	mLocked = aShouldLock;
	if(mLocked) {
		glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
		glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}
