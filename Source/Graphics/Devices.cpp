#include "Devices.h"

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

	//data gather
	for (int i = 0; i < deviceCount; i++) {
		DeviceData& device = gDevices[i];
		device.mPhysicalDevice = devices[i];
		device.mSurfaceUsed = mSurface;

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
				vkGetPhysicalDeviceSurfaceSupportKHR(device.mPhysicalDevice, q, device.mSurfaceUsed, &presentSupport);
				if (presentSupport) {
					device.mQueue.mQueueFamilies[q].mPresent = true;
				}
			}

			auto GetQueueFamily = [&](const DeviceData::Queue::QueueTypes aType, uint8_t aQueueFamily) {
				const uint8_t queueIndex = static_cast<uint8_t>(aType);
				//cant seem to use a union?
				DeviceData::Queue::QueueIndex& queue = *((&device.mQueue.mGraphicsQueue)+queueIndex);
				DeviceData::Queue::QueueFamilys& family = device.mQueue.mQueueFamilies[aQueueFamily];
				if(queue.mQueueFamily == -1 && family.mQueueTypeFlags & 1 << queueIndex && family.mProperties.queueCount != family.mUsedQueues){
					queue.mQueueFamily = aQueueFamily;
					family.mUsedQueues++;
				}
			};

			//GetQueueFamily(DeviceData::Queue::QueueTypes::GRAPHICS, 0);

			for (int q = 0; q < (int)DeviceData::Queue::QueueTypes::NUM; q++) {
				for (int f = 0; f < queueFamilyCount; f++) {
					GetQueueFamily((DeviceData::Queue::QueueTypes)q,f);
				}
			}
		}

		//swapchain check
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.mPhysicalDevice, device.mSurfaceUsed,
				&device.mSwapchain.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device.mPhysicalDevice, device.mSurfaceUsed, &formatCount,
				nullptr);

			if (formatCount != 0) {
				device.mSwapchain.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device.mPhysicalDevice, device.mSurfaceUsed, &formatCount,
					device.mSwapchain.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device.mPhysicalDevice, device.mSurfaceUsed,
				&presentModeCount, nullptr);

			if (presentModeCount != 0) {
				device.mSwapchain.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(
					device.mPhysicalDevice, device.mSurfaceUsed, &presentModeCount, device.mSwapchain.presentModes.data());
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
	std::fill_n(queuePriority, 10, 1.0f);
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

void Devices::Destroy() {
}

void Devices::CreateCommandPools() {

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	createInfo.queueFamilyIndex = GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueueFamily;
	vkCreateCommandPool(GetPrimaryDevice(), &createInfo, GetAllocationCallback(), &mCommandPoolGraphics);

	createInfo.queueFamilyIndex = GetPrimaryDeviceData().mQueue.mComputeQueue.mQueueFamily;
	vkCreateCommandPool(GetPrimaryDevice(), &createInfo, GetAllocationCallback(), &mCommandPoolCompute);

	createInfo.queueFamilyIndex = GetPrimaryDeviceData().mQueue.mTransferQueue.mQueueFamily;
	vkCreateCommandPool(GetPrimaryDevice(), &createInfo, GetAllocationCallback(), &mCommandPoolTransfer);

}
void Devices::CreateCommandBuffers(const uint8_t aNumBuffers) {
	mGraphicsCommandBuffers.resize(aNumBuffers);
	mComputeCommandBuffers.resize(aNumBuffers);
	mTransferCommandBuffers.resize(aNumBuffers);

	VkCommandBufferAllocateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	createInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	createInfo.commandBufferCount = aNumBuffers;

	createInfo.commandPool = mCommandPoolGraphics;
	vkAllocateCommandBuffers(GetPrimaryDevice(), &createInfo, mGraphicsCommandBuffers.data());
	createInfo.commandPool = mCommandPoolCompute;
	vkAllocateCommandBuffers(GetPrimaryDevice(), &createInfo, mComputeCommandBuffers.data());
	createInfo.commandPool = mCommandPoolTransfer;
	vkAllocateCommandBuffers(GetPrimaryDevice(), &createInfo, mTransferCommandBuffers.data());

}

const DeviceData& Devices::GetPrimaryDeviceData() const {
	return gDevices[0];
}

const VkDevice Devices::GetPrimaryDevice() const {
	return GetPrimaryDeviceData().mDevice;
}

const VkPhysicalDevice Devices::GetPrimaryPhysicalDevice() const {
	return GetPrimaryDeviceData().mPhysicalDevice;
}

const VkSurfaceKHR Devices::GetPrimarySurface() const {
	return GetPrimaryDeviceData().mSurfaceUsed;
}
