#pragma once

#include <vector>
#include <vulkan/vulkan.h>

struct DeviceData {
	VkPhysicalDevice mPhysicalDevice;
	VkDevice mDevice;
	struct Queue {
		struct QueueFamilys {
			VkQueueFamilyProperties mProperties;
			bool mGraphics : 1;
			bool mCompute : 1;
			bool mTransfer : 1;
			bool mPresent : 1;
		};
		std::vector<QueueFamilys> mQueueFamilies;
		struct QueueIndex {
			int8_t mQueueFamily = -1;
			VkQueue mQueue;
		};
		QueueIndex mGraphicsQueue;
		QueueIndex mComputeQueue;
		QueueIndex mTransferQueue;
		QueueIndex mPresentQueue;
	} mQueue;

	struct SwapChain {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats{};
		std::vector<VkPresentModeKHR> presentModes{};
	} mSwapChain;

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

	const DeviceData& GetPrimaryDeviceData() const;
	const VkDevice GetPrimaryDevice() const;
	const VkPhysicalDevice GetPrimaryPhysicalDevice() const;
	const VkSurfaceKHR GetPrimarySurface() const;
private:
	VkSurfaceKHR mSurface;
};