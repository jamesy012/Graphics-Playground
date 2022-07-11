#include "Devices.h"

#include <cstring> //memset

#include "Graphics.h"
#include "PlatformDebug.h"

#if defined(ENABLE_XR)
#	include "VRGraphics.h"
#endif

// clang-format off
const char* gDeviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME
#if PLATFORM_APPLE
								   , "VK_KHR_portability_subset"
#endif
								   , VK_KHR_MULTIVIEW_EXTENSION_NAME};
// clang-format on

extern VkInstance gVkInstance;
extern Graphics* gGraphics;

std::vector<DeviceData> gDevices(0);

bool Devices::Setup() {
	VkResult result;
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(gVkInstance, &deviceCount, nullptr);
	if(deviceCount == 0) {
		ASSERT(false); // failed to find GPUs with vulkan support
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(gVkInstance, &deviceCount, devices.data());

	gDevices.resize(deviceCount);

	//data gather
	for(int i = 0; i < deviceCount; i++) {
		DeviceData& device = gDevices[i];
		device.mPhysicalDevice = devices[i];
		device.mSurfaceUsed = mSurface;

		//queues
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device.mPhysicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			device.mQueue.mQueueFamilies.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device.mPhysicalDevice, &queueFamilyCount, queueFamilies.data());

			for(int q = 0; q < queueFamilyCount; q++) {
				device.mQueue.mQueueFamilies[q].mProperties = queueFamilies[q];

				if(queueFamilies[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					device.mQueue.mQueueFamilies[q].mGraphics = true;
				}
				if(queueFamilies[q].queueFlags & VK_QUEUE_COMPUTE_BIT) {
					device.mQueue.mQueueFamilies[q].mCompute = true;
				}
				if(queueFamilies[q].queueFlags & VK_QUEUE_TRANSFER_BIT) {
					device.mQueue.mQueueFamilies[q].mTransfer = true;
				}
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device.mPhysicalDevice, q, device.mSurfaceUsed, &presentSupport);
				if(presentSupport) {
					device.mQueue.mQueueFamilies[q].mPresent = true;
				}
			}

			auto GetQueueFamily = [&](const DeviceData::Queue::QueueTypes aType, uint8_t aQueueFamily) {
				const uint8_t queueIndex = static_cast<uint8_t>(aType);
				//cant seem to use a union?
				DeviceData::Queue::QueueIndex& queue = *((&device.mQueue.mGraphicsQueue) + queueIndex);
				DeviceData::Queue::QueueFamilys& family = device.mQueue.mQueueFamilies[aQueueFamily];
				if(queue.mQueueFamily == -1 && family.mQueueTypeFlags & 1 << queueIndex && family.mProperties.queueCount != family.mUsedQueues) {
					queue.mQueueFamily = aQueueFamily;
					family.mUsedQueues++;
				}
			};

			//GetQueueFamily(DeviceData::Queue::QueueTypes::GRAPHICS, 0);

			for(int q = 0; q < (int)DeviceData::Queue::QueueTypes::NUM; q++) {
				for(int f = 0; f < queueFamilyCount; f++) {
					GetQueueFamily((DeviceData::Queue::QueueTypes)q, f);
				}
			}
		}

		//swapchain check
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.mPhysicalDevice, device.mSurfaceUsed, &device.mSwapchain.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device.mPhysicalDevice, device.mSurfaceUsed, &formatCount, nullptr);

			if(formatCount != 0) {
				device.mSwapchain.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device.mPhysicalDevice, device.mSurfaceUsed, &formatCount, device.mSwapchain.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device.mPhysicalDevice, device.mSurfaceUsed, &presentModeCount, nullptr);

			if(presentModeCount != 0) {
				device.mSwapchain.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(
					device.mPhysicalDevice, device.mSurfaceUsed, &presentModeCount, device.mSwapchain.presentModes.data());
			}
		}

		//extension check
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device.mPhysicalDevice, nullptr, &extensionCount, nullptr);

			device.mExtensions.resize(extensionCount);
			vkEnumerateDeviceExtensionProperties(device.mPhysicalDevice, nullptr, &extensionCount, device.mExtensions.data());

			vkEnumerateDeviceLayerProperties(device.mPhysicalDevice, &extensionCount, nullptr);
			device.mLayers.resize(extensionCount);
			vkEnumerateDeviceLayerProperties(device.mPhysicalDevice, &extensionCount, device.mLayers.data());
		}

		//extra info
		{
			VulkanResursiveSetpNext(&device.mDeviceFeatures, &device.mDeviceMultiViewFeatures);
			device.mDeviceMultiViewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
			VulkanResursiveSetpNext(&device.mDeviceProperties, &device.mDeviceMultiViewProperties);
			device.mDeviceMultiViewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;

			VulkanResursiveSetpNext(&device.mDeviceFeatures, &device.mDeviceDescriptorIndexingFeatures);
			device.mDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
			VulkanResursiveSetpNext(&device.mDeviceProperties, &device.mDeviceDescriptorIndexingProperties);
			device.mDeviceDescriptorIndexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;

			device.mDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			vkGetPhysicalDeviceFeatures2(device.mPhysicalDevice, &device.mDeviceFeatures);
			device.mDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkGetPhysicalDeviceProperties2(device.mPhysicalDevice, &device.mDeviceProperties);
		}
	}

	int selectedDeviceIndex = 0;
#if defined(ENABLE_XR)
	VkPhysicalDevice requestedDevice = gVrGraphics->GetRequestedDevice();
	selectedDeviceIndex = -1;
	for(int i = 0; i < gDevices.size(); i++) {
		if(gDevices[i].mPhysicalDevice == requestedDevice) {
			selectedDeviceIndex = i;
			break;
		}
	}
	ASSERT(selectedDeviceIndex != -1);
#endif
	DeviceData& selectedDevice = gDevices[selectedDeviceIndex];

	LOGGER::Formated("Selected GPU: {} \n", selectedDevice.mDeviceProperties.properties.deviceName);

	VkDeviceCreateInfo deviceInfo {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	std::vector<const char*> extensions;
	std::vector<std::string> extensionsTemp;
	extensions.assign(gDeviceExtensions, gDeviceExtensions + std::size(gDeviceExtensions));

#if defined(ENABLE_XR)
	std::vector<std::string> xrExtensions = gVrGraphics->GetVulkanDeviceExtensions();
	extensionsTemp.insert(extensionsTemp.end(), xrExtensions.begin(), xrExtensions.end());
#endif

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
		for(int i = 0; i < numQueues; i++) {
			queueInfo.queueCount = uniqueQueues[i];
			queueInfo.queueFamilyIndex = i;
			if(queueInfo.queueCount != 0) {
				queues.push_back(queueInfo);
			}
		}
		delete[] uniqueQueues;
	}

	deviceInfo.queueCreateInfoCount = queues.size();
	deviceInfo.pQueueCreateInfos = queues.data();

	VkPhysicalDeviceFeatures2 deviceFeatures {};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.features.samplerAnisotropy = VK_TRUE;
	deviceFeatures.features.multiDrawIndirect = VK_TRUE;

	if(selectedDevice.IsMultiViewAvailable()) {
		VkPhysicalDeviceMultiviewFeatures multiView = {};
		multiView.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
		multiView.multiview = VK_TRUE;
		VulkanResursiveSetpNext(&deviceFeatures, &multiView);
		mMultiviewEnabled = true;
	} else {
		ASSERT(false);
	}
	if(selectedDevice.IsBindlessDescriptorsAvailable()) {
		VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexing = {};
		descriptorIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexing.descriptorBindingPartiallyBound = VK_TRUE;
		descriptorIndexing.runtimeDescriptorArray = VK_TRUE;
		descriptorIndexing.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		descriptorIndexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
		VulkanResursiveSetpNext(&deviceFeatures, &descriptorIndexing);
		extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		mBindlessDescriptorsEnabled = true;
	} else {
		ASSERT(false);
	}

	deviceInfo.pEnabledFeatures = nullptr;
	deviceInfo.pNext = &deviceFeatures;

	// copy over any temporary strings
	for(int i = 0; i < extensionsTemp.size(); i++) {
		extensions.push_back(extensionsTemp[i].c_str());
	}

	deviceInfo.enabledExtensionCount = extensions.size();
	deviceInfo.ppEnabledExtensionNames = extensions.data();

	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = nullptr;

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
	vkFreeCommandBuffers(GetPrimaryDevice(), mCommandPoolGraphics, mGraphicsCommandBuffers.size(), mGraphicsCommandBuffers.data());
	vkFreeCommandBuffers(GetPrimaryDevice(), mCommandPoolCompute, mComputeCommandBuffers.size(), mComputeCommandBuffers.data());
	vkFreeCommandBuffers(GetPrimaryDevice(), mCommandPoolTransfer, mTransferCommandBuffers.size(), mTransferCommandBuffers.data());
	vkDestroyCommandPool(GetPrimaryDevice(), mCommandPoolGraphics, GetAllocationCallback());
	vkDestroyCommandPool(GetPrimaryDevice(), mCommandPoolCompute, GetAllocationCallback());
	vkDestroyCommandPool(GetPrimaryDevice(), mCommandPoolTransfer, GetAllocationCallback());

	for(int i = 0; i < gDevices.size(); i++) {
		vkDestroyDevice(gDevices[i].mDevice, GetAllocationCallback());
	}
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

	SetVkName(VK_OBJECT_TYPE_COMMAND_POOL, mCommandPoolGraphics, "Command Pool Graphics");
	SetVkName(VK_OBJECT_TYPE_COMMAND_POOL, mCommandPoolCompute, "Command Pool Compute");
	SetVkName(VK_OBJECT_TYPE_COMMAND_POOL, mCommandPoolTransfer, "Command Pool Transfer");
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
