#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <tracy/TracyVulkan.hpp>

#include <vector>
#include <mutex>
#include <functional>

#include "Engine/IGraphicsBase.h"

#include "PlatformDebug.h"
#include "Engine/Callback.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Buffer.h"

//this should be in graphics section?
#include "Engine/AllocationCallbacks.h"

class Window;
class Devices;
class Swapchain;

class MaterialManager;
class VRGraphics;

#pragma region Vulkan Helpers

struct OneTimeCommandBuffer {
	Buffer mDataBuffer;

	operator VkCommandBuffer() {
		return mBuffer;
	}

protected:
	friend class VulkanGraphics;
	VkCommandBuffer mBuffer;
	//VkFence mFence;
};

void SetVkName(VkObjectType aType, uint64_t aObject, const char* aName);
template<typename T>
inline void SetVkName(VkObjectType aType, T aObject, const char* aName) {
	SetVkName(aType, (uint64_t)aObject, aName);
}
template<typename T>
inline void SetVkName(VkObjectType aType, T aObject, const std::string aName) {
	SetVkName(aType, (uint64_t)aObject, aName.c_str());
}

static void VulkanResursiveSetpNext(void* dst, void* pNext) {
	if(dst == nullptr) {
		return;
	}
	VkBaseInStructure* dstStruct = (VkBaseInStructure*)dst;
	while(dstStruct->pNext != nullptr) {
		dstStruct = (VkBaseInStructure*)dstStruct->pNext;
	}
	dstStruct->pNext = (VkBaseInStructure*)pNext;
}

//Turns off and on validation messages
//XR Runtime seems to have issues?
void VulkanValidationMessage(int32_t aMessageId, bool aEnabled);

#define VALIDATEVK(function)                  \
	{                                         \
		const VkResult vkResult = (function); \
		ASSERT(vkResult == VK_SUCCESS);       \
	}

#pragma endregion

//Controls Vulkan startup/Shutdown
//swapchains
//contains genreal render information
class VulkanGraphics : public IGraphicsBase {
public:
	VulkanGraphics() {};
	~VulkanGraphics() {};

	//~~~ creation/destruction
	// starts vulkan
	bool Startup() override;
	// starts to set up vulkan objects, Device/Swapchain/Command buffers
	bool Initalize() override;
	bool Destroy() override;

	void AddWindow(Window* aWindow) override;

	//~~~ Frame Management
	void StartNewFrame() override;
	void StartGraphicsFrame() override;
	void EndFrame() override;

	const uint32_t GetCurrentImageIndex() const;
	const uint32_t GetFrameCount() const {
		return mFrameCounter;
	}

	VkCommandBuffer GetCurrentGraphicsCommandBuffer() const;
	const Framebuffer& GetCurrentFrameBuffer() const {
		return mFramebuffer[GetCurrentImageIndex()];
	};
#if defined(ENABLE_XR)
	const Framebuffer& GetCurrentXrFrameBuffer(uint8_t aEye) const;
#endif

	OneTimeCommandBuffer AllocateGraphicsCommandBuffer(bool aBegin = true);
	// submits and finish's the command buffer
	void EndGraphicsCommandBuffer(OneTimeCommandBuffer aBuffer, bool aEnd = true);

	//~~~ Helpers
	const VkInstance GetVkInstance() const;
	const VkDevice GetVkDevice() const;
	const VkPhysicalDevice GetVkPhysicalDevice() const;
	const Devices* GetMainDevice() const;
	const Swapchain* GetMainSwapchain() const;

	//number of final views we want to render to
	const uint8_t GetNumActiveViews() const {
#if defined(ENABLE_XR)
		return 2;
#endif
		return 1;
	}

	static const bool IsFormatDepth(VkFormat aFormat);
	static const VkFormat GetDeafultDepthFormat();
	static const VkFormat GetDeafultColorFormat();
	const VkFormat GetSwapchainFormat() const;
#if defined(ENABLE_XR)
	const VkFormat GetXRSwapchainFormat() const;
#endif

	const VmaAllocator GetAllocator() const {
		return mAllocator;
	}

	const VkDescriptorPool GetDesciptorPool() const {
		return mDescriptorPool;
	};
	const VkSampler GetDefaultSampler() const {
		return mSampler;
	};

	const ImageSize GetDesiredSize() const;

	MaterialManager* GetMaterialManager() const {
		return mMaterialManager;
	}

	Callback<void()> mResizeMessage;

private:
	bool CreateInstance();

	RenderPass mRenderPass = {};
	Framebuffer mFramebuffer[3] = {};
#if defined(ENABLE_XR)
	RenderPass mXrRenderPass = {};
	Framebuffer mXrFramebuffer[2][3] = {};
#endif

	bool HasInstanceExtension(const char* aExtension) const;
	bool HasInstanceLayer(const char* aLayer) const;

	void ExecuteOneTimeCommandBuffers();

	std::vector<VkLayerProperties> mInstanceLayers = {};
	std::vector<std::vector<VkExtensionProperties>> mLayerExtensions = {};

	std::vector<Window*> mSurfaces = {};

	Devices* mDevicesHandler = nullptr;
	Swapchain* mSwapchain = nullptr;

	VmaAllocator mAllocator = VK_NULL_HANDLE;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkSampler mSampler = VK_NULL_HANDLE;

	MaterialManager* mMaterialManager = nullptr;

	uint32_t mFrameCounter = 0;

	std::vector<OneTimeCommandBuffer> mBuffersToSubmit = {};
	TracyLockable(std::mutex, mBuffersToSubmitMutex);
	TracyLockable(std::mutex, mCommandPoolMutex);
	//std::mutex mBuffersToSubmitMutex;
	//std::mutex mCommandPoolMutex;
};

extern VulkanGraphics* gGraphics;
typedef VulkanGraphics Graphics;
