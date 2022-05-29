#include "Window.h"

#include <vulkan/vulkan.h>

#include "Graphics.h"
#include "PlatformDebug.h"

#if PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif PLATFORM_APPLE
#include <GLFW/glfw3.h>
#endif

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

void* Window::CreateSurface() {
	ASSERT(gVkInstance != VK_NULL_HANDLE);
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	const VkResult result = glfwCreateWindowSurface(gVkInstance, mWindow, GetAllocationCallback(), &surface);
	if (result != VK_SUCCESS) {
		ASSERT(false);// Failed to create surface
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
	glfwPollEvents();
}

const char** Window::GetGLFWVulkanExtentensions(uint32_t* aCount) {
	return glfwGetRequiredInstanceExtensions(aCount);
}

