#pragma once

#include <vector>
#include <vulkan/vulkan.h>

struct DeviceData {
	VkPhysicalDevice mPhysicalDevice;
	VkDevice mDevice;
	struct Queue {
		enum class QueueTypes
		{
			GRAPHICS,
			COMPUTE,
			TRANSFER,
			PRESENT,
			NUM
		};
		struct QueueFamilys {
			VkQueueFamilyProperties mProperties;
			union {
				struct {
					bool mGraphics : 1;
					bool mCompute : 1;
					bool mTransfer : 1;
					bool mPresent : 1;
				};
				uint8_t mQueueTypeFlags;
			};
			uint8_t mUsedQueues;
		};
		std::vector<QueueFamilys> mQueueFamilies;
		struct QueueIndex {
			int8_t mQueueFamily = -1;
			VkQueue mQueue		= VK_NULL_HANDLE;
		};
		//union {
		//struct {
		QueueIndex mGraphicsQueue;
		QueueIndex mComputeQueue;
		QueueIndex mTransferQueue;
		QueueIndex mPresentQueue;
		//};
		//	QueueIndex mQueues[(int)QueueTypes::NUM];
		//};
	} mQueue;

	struct SwapChain {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats {};
		std::vector<VkPresentModeKHR> presentModes {};
	} mSwapchain;

	std::vector<VkExtensionProperties> mExtensions;
	std::vector<VkLayerProperties> mLayers;

	VkPhysicalDeviceFeatures mDeviceFeatures;
	VkPhysicalDeviceProperties mDeviceProperties;

	VkSurfaceKHR mSurfaceUsed;
};

class Devices {
public:
	Devices(const VkSurfaceKHR aSurface) : mSurface(aSurface) {};
	bool Setup();
	void Destroy();

	void CreateCommandPools();
	void CreateCommandBuffers(const uint8_t aNumBuffers);

	const DeviceData& GetPrimaryDeviceData() const;
	const VkDevice GetPrimaryDevice() const;
	const VkPhysicalDevice GetPrimaryPhysicalDevice() const;
	const VkSurfaceKHR GetPrimarySurface() const;

	const VkCommandBuffer GetGraphicsCB(const uint8_t aIndex) const {
		return mGraphicsCommandBuffers[aIndex];
	}
	const VkCommandPool GetGraphicsPool() const {
		return mCommandPoolGraphics;
	}

private:
	VkSurfaceKHR mSurface;

	//should be in DeviceData? / Combine below
	VkCommandPool mCommandPoolGraphics;
	VkCommandPool mCommandPoolCompute;
	VkCommandPool mCommandPoolTransfer;

	std::vector<VkCommandBuffer> mGraphicsCommandBuffers;
	std::vector<VkCommandBuffer> mComputeCommandBuffers;
	std::vector<VkCommandBuffer> mTransferCommandBuffers;
};