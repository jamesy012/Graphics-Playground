#include "Devices.h"

#include <vulkan/vulkan.h>
#include <vector>

#include "Graphics.h"
#include "PlatformDebug.h"

const char* gDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if PLATFORM_APPLE
	"VK_KHR_portability_subset"
#endif
};

extern VkInstance gVkInstance;
extern Graphics* gGraphics;

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
};
std::vector<DeviceData> gDevices(0);

bool Devices::Setup() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(gVkInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		ASSERT(false);// failed to find GPUs with vulkan support
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(gVkInstance, &deviceCount, devices.data());

	gDevices.resize(deviceCount);

	VkSurfaceKHR surface = gGraphics->mSurfaces[0];

	//data gather
	for (int i = 0; i < deviceCount; i++) {
		DeviceData& device = gDevices[i];
		device.mPhysicalDevice = devices[i];

		//queues
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device.mPhysicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			device.mQueue.mQueueFamilies.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device.mPhysicalDevice, &queueFamilyCount,
				queueFamilies.data());

			for (int q = 0; q < queueFamilyCount; q++) {
				device.mQueue.mQueueFamilies[q].mProperties = queueFamilies[q];

				if (queueFamilies[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					device.mQueue.mQueueFamilies[q].mGraphics = true;
				}
				if (queueFamilies[q].queueFlags & VK_QUEUE_COMPUTE_BIT) {
					device.mQueue.mQueueFamilies[q].mCompute = true;
				}
				if (queueFamilies[q].queueFlags & VK_QUEUE_TRANSFER_BIT) {
					device.mQueue.mQueueFamilies[q].mTransfer = true;
				}
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device.mPhysicalDevice, q, surface, &presentSupport);
				if (presentSupport) {
					device.mQueue.mQueueFamilies[q].mPresent = true;
				}
			}

			for (int q = 0; q < queueFamilyCount; q++) {
				if (device.mQueue.mGraphicsQueue.mQueueFamily == -1 && device.mQueue.mQueueFamilies[q].mGraphics) {
					device.mQueue.mGraphicsQueue.mQueueFamily = q;
				}
				if (device.mQueue.mComputeQueue.mQueueFamily == -1 && device.mQueue.mQueueFamilies[q].mCompute) {
					device.mQueue.mComputeQueue.mQueueFamily = q;
				}
				if (device.mQueue.mTransferQueue.mQueueFamily == -1 && device.mQueue.mQueueFamilies[q].mTransfer) {
					device.mQueue.mTransferQueue.mQueueFamily = q;
				}
				if (device.mQueue.mPresentQueue.mQueueFamily == -1 && device.mQueue.mQueueFamilies[q].mPresent) {
					device.mQueue.mPresentQueue.mQueueFamily = q;
				}
			}
		}

		//swapchain check
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.mPhysicalDevice, surface,
				&device.mSwapChain.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device.mPhysicalDevice, surface, &formatCount,
				nullptr);

			if (formatCount != 0) {
				device.mSwapChain.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device.mPhysicalDevice, surface, &formatCount,
					device.mSwapChain.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device.mPhysicalDevice, surface,
				&presentModeCount, nullptr);

			if (presentModeCount != 0) {
				device.mSwapChain.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(
					device.mPhysicalDevice, surface, &presentModeCount, device.mSwapChain.presentModes.data());
			}
		}

		//extension check
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device.mPhysicalDevice, nullptr, &extensionCount,
				nullptr);

			device.mExtensions.resize(extensionCount);
			vkEnumerateDeviceExtensionProperties(device.mPhysicalDevice, nullptr, &extensionCount,
				device.mExtensions.data());

			vkEnumerateDeviceLayerProperties(device.mPhysicalDevice, &extensionCount,
				nullptr);
			device.mLayers.resize(extensionCount);
			vkEnumerateDeviceLayerProperties(device.mPhysicalDevice, &extensionCount,
				device.mLayers.data());
		}

		//extra info
		{
			vkGetPhysicalDeviceFeatures(device.mPhysicalDevice, &device.mDeviceFeatures);
			vkGetPhysicalDeviceProperties(device.mPhysicalDevice, &device.mDeviceProperties);
		}
	}

	DeviceData& selectedDevice = gDevices[0];

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	std::vector<const char*> extensions;
	extensions.assign(gDeviceExtensions, gDeviceExtensions + std::size(gDeviceExtensions));

	std::vector<VkDeviceQueueCreateInfo> queues;
	float queuePriority[10];
	std::fill_n(queuePriority, sizeof(queuePriority), 1.0f);
	{
		int numQueues = selectedDevice.mQueue.mQueueFamilies.size();
		uint8_t* uniqueQueues = new uint8_t[numQueues];
		memset(uniqueQueues, 0, numQueues * sizeof(uint8_t));
		uniqueQueues[selectedDevice.mQueue.mGraphicsQueue.mQueueFamily]++;
		uniqueQueues[selectedDevice.mQueue.mComputeQueue.mQueueFamily]++;
		uniqueQueues[selectedDevice.mQueue.mTransferQueue.mQueueFamily]++;
		uniqueQueues[selectedDevice.mQueue.mPresentQueue.mQueueFamily]++;

		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pQueuePriorities = queuePriority;
		for (int i = 0; i < numQueues; i++) {
			queueInfo.queueCount = uniqueQueues[i];
			queueInfo.queueFamilyIndex = i;
			if (queueInfo.queueCount != 0) {
				queues.push_back(queueInfo);
			}
		}
		delete[] uniqueQueues;
	}
	deviceInfo.queueCreateInfoCount = queues.size();
	deviceInfo.pQueueCreateInfos = queues.data();

	deviceInfo.enabledExtensionCount = extensions.size();
	deviceInfo.ppEnabledExtensionNames = extensions.data();

	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = nullptr;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.multiDrawIndirect = VK_TRUE;
	deviceInfo.pEnabledFeatures = &deviceFeatures;

	vkCreateDevice(selectedDevice.mPhysicalDevice, &deviceInfo, GetAllocationCallback(), &selectedDevice.mDevice);

	//Get Queue
	{
		int numQueues = selectedDevice.mQueue.mQueueFamilies.size();
		uint8_t* uniqueQueues = new uint8_t[numQueues];
		memset(uniqueQueues, 0, numQueues * sizeof(uint8_t));
		auto getQueue = [&](DeviceData::Queue::QueueIndex& queue) {
			int family = queue.mQueueFamily;
			vkGetDeviceQueue(selectedDevice.mDevice, family, uniqueQueues[family]++, &queue.mQueue);
		};
		getQueue(selectedDevice.mQueue.mGraphicsQueue);
		getQueue(selectedDevice.mQueue.mComputeQueue);
		getQueue(selectedDevice.mQueue.mTransferQueue);
		getQueue(selectedDevice.mQueue.mPresentQueue);
		delete[] uniqueQueues;
	}

	return true;
}
