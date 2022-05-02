#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>

class Window;
class Devices;
class SwapChain;

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

	//starts vulkan
	bool StartUp();
	//starts to set up vulkan objects, Device/Swapchain/Command buffers
	bool Initalize();
	bool Destroy();

	void AddWindow(Window* aWindow);

	std::vector<VkSurfaceKHR> mSurfaces;

	const VkDevice GetDevice() const;
	const VkPhysicalDevice GetPhysicalDevice() const;
private:
	bool CreateInstance();

	bool HasInstanceExtension(const char* aExtension) const;
	bool HasInstanceLayer(const char* aLayer) const;

	std::vector<VkLayerProperties> mInstanceLayers;
	std::vector<VkLayerProperties> mDeviceLayers;
	std::vector<VkExtensionProperties> mInstanceExtensions;
	std::vector<VkExtensionProperties> mDeviceExtensions;


	Devices* mDevicesHandler;
	SwapChain* mSwapChain;

  	VmaAllocator mAllocator;

};