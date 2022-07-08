#pragma once

struct GLFWwindow;
#include <stdint.h>

class Window {
public:
	void Create(const int aWidth, const int aHeight, const char* aTitle);
	void Destroy();

	void* CreateSurface();
	void DestroySurface();

	bool ShouldClose();
	void WaitEvents();
	void Update();

	static const char** GetGLFWVulkanExtentensions(uint32_t* aCount);

	//VkSurfaceKHR
	const void* GetSurface() const {
		return mSurface;
	}
	GLFWwindow* GetWindow() const {
		return mWindow;
	}

	void GetSize(int* aWidth, int* aHeight) const;

	bool HasFocus() const;
	void SetLock(const bool aShouldLock);
	bool IsLocked() const {
		return mLocked;
	}
private:
	GLFWwindow* mWindow = nullptr;
	//VkSurfaceKHR
	void* mSurface = nullptr;

	bool mLocked = false;
};