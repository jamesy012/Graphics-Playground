#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>

class Window;
class Devices;
class Swapchain;

static VkAllocationCallbacks* CreateAllocationCallbacks() {
	VkAllocationCallbacks callback;
	callback.pUserData = nullptr;
	callback.pfnAllocation = nullptr;
	callback.pfnReallocation = nullptr;
	callback.pfnFree = nullptr;
	callback.pfnInternalAllocation = nullptr;
	callback.pfnInternalFree = nullptr;
	return nullptr;
}

static VkAllocationCallbacks* GetAllocationCallback() {
	static VkAllocationCallbacks* allocationCallback = CreateAllocationCallbacks();
	return allocationCallback;
}

class Graphics {
public:
	Graphics() {};
	~Graphics() {};

//~~~ creation/destruction
	//starts vulkan
	bool StartUp();
	//starts to set up vulkan objects, Device/Swapchain/Command buffers
	bool Initalize();
	bool Destroy();

	void AddWindow(Window* aWindow);

//~~~ Frame Management
	void StartNewFrame();
	void EndFrame();

	const uint32_t GetCurrentImageIndex() const;
	const uint32_t GetFrameCount() const { return mFrameCounter; }

	VkCommandBuffer GetCurrentGraphicsCommandBuffer() const;

//~~~ Helpers
	const VkDevice GetVkDevice() const;
	const VkPhysicalDevice GetVkPhysicalDevice() const;
	const Devices* GetMainDevice() const;
	const Swapchain* GetMainSwapchain() const;
private:
	bool CreateInstance();

	bool HasInstanceExtension(const char* aExtension) const;
	bool HasInstanceLayer(const char* aLayer) const;

	std::vector<VkLayerProperties> mInstanceLayers;
	std::vector<VkLayerProperties> mDeviceLayers;
	std::vector<VkExtensionProperties> mInstanceExtensions;
	std::vector<VkExtensionProperties> mDeviceExtensions;

	std::vector<VkSurfaceKHR> mSurfaces;

	Devices* mDevicesHandler;
	Swapchain* mSwapchain;

  	VmaAllocator mAllocator;

	uint32_t mFrameCounter = 0;

};

extern Graphics* gGraphics;
