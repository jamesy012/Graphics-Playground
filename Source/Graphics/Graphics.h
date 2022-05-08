#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>

#include "Image.h"

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

struct OneTimeCommandBuffer {
	VkCommandBuffer mBuffer;
	VkFence mFence;
	operator VkCommandBuffer() { return mBuffer; }
};

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

	OneTimeCommandBuffer AllocateGraphicsCommandBuffer();
	//submits and finish's the command buffer
	void EndGraphicsCommandBuffer(OneTimeCommandBuffer aBuffer);

//~~~ Helpers
	const VkDevice GetVkDevice() const;
	const VkPhysicalDevice GetVkPhysicalDevice() const;
	const Devices* GetMainDevice() const;
	const Swapchain* GetMainSwapchain() const;

	const VkFormat GetMainFormat() const;
	const VmaAllocator GetAllocator() const { return mAllocator; }
private:
	bool CreateInstance();
#if defined(ENABLE_IMGUI)
	bool CreateImGui();

	Image mImGuiFontImage;

#endif

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
