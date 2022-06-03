#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <vector>

#include "Buffer.h"
#include "Framebuffer.h"
#include "Image.h"
#include "Material.h"
#include "Pipeline.h"
#include "RenderPass.h"

class Window;
class Devices;
class Swapchain;

static VkAllocationCallbacks* CreateAllocationCallbacks() {
	VkAllocationCallbacks callback;
	callback.pUserData			   = nullptr;
	callback.pfnAllocation		   = nullptr;
	callback.pfnReallocation	   = nullptr;
	callback.pfnFree			   = nullptr;
	callback.pfnInternalAllocation = nullptr;
	callback.pfnInternalFree	   = nullptr;
	return nullptr;
}

static VkAllocationCallbacks* GetAllocationCallback() {
	static VkAllocationCallbacks* allocationCallback = CreateAllocationCallbacks();
	return allocationCallback;
}

struct OneTimeCommandBuffer {
	VkCommandBuffer mBuffer;
	VkFence mFence;
	operator VkCommandBuffer() {
		return mBuffer;
	}
};

void SetVkName(VkObjectType aType, uint64_t aObject, const char* aName);
template<typename T> inline void SetVkName(VkObjectType aType, T aObject, const char* aName) {
	SetVkName(aType, (uint64_t)aObject, aName);
}
template<typename T> inline void SetVkName(VkObjectType aType, T aObject, const std::string aName) {
	SetVkName(aType, (uint64_t)aObject, aName.c_str());
}

class Graphics {
public:
	Graphics() {};
	~Graphics() {};

	//~~~ creation/destruction
	// starts vulkan
	bool StartUp();
	// starts to set up vulkan objects, Device/Swapchain/Command buffers
	bool Initalize();
	bool Destroy();

	void AddWindow(Window* aWindow);

	//~~~ Frame Management
	void StartNewFrame();
	void EndFrame();

	const uint32_t GetCurrentImageIndex() const;
	const uint32_t GetFrameCount() const {
		return mFrameCounter;
	}

	VkCommandBuffer GetCurrentGraphicsCommandBuffer() const;

	OneTimeCommandBuffer AllocateGraphicsCommandBuffer();
	// submits and finish's the command buffer
	void EndGraphicsCommandBuffer(OneTimeCommandBuffer aBuffer);

	//~~~ Helpers
	const VkDevice GetVkDevice() const;
	const VkPhysicalDevice GetVkPhysicalDevice() const;
	const Devices* GetMainDevice() const;
	const Swapchain* GetMainSwapchain() const;

	const VkFormat GetMainFormat() const;
	const VmaAllocator GetAllocator() const {
		return mAllocator;
	}

	const VkDescriptorPool GetDesciptorPool() const {
		return mDescriptorPool;
	};
	const VkSampler GetDefaultSampler() const {
		return mSampler;
	};

private:
	bool CreateInstance();
#if defined(ENABLE_IMGUI)
	bool CreateImGui();
	void RenderImGui(VkCommandBuffer aBuffer);

	Image mImGuiFontImage;
	Pipeline mImGuiPipeline;
	Buffer mImGuiVertBuffer;
	Buffer mImGuiIndexBuffer;
	MaterialBase mImGuiFontMaterialBase;
	Material mImGuiFontMaterial;
#endif

	RenderPass mRenderPass;
	Framebuffer mFramebuffer[3];

	bool HasInstanceExtension(const char* aExtension) const;
	bool HasInstanceLayer(const char* aLayer) const;

	std::vector<VkLayerProperties> mInstanceLayers;
	std::vector<VkLayerProperties> mDeviceLayers;
	std::vector<VkExtensionProperties> mInstanceExtensions;
	std::vector<VkExtensionProperties> mDeviceExtensions;

	std::vector<Window*> mSurfaces;

	Devices* mDevicesHandler;
	Swapchain* mSwapchain;

	VmaAllocator mAllocator;

	VkDescriptorPool mDescriptorPool;
	VkSampler mSampler;

	uint32_t mFrameCounter = 0;
};

extern Graphics* gGraphics;
