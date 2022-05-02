#include "Graphics.h"

#include "PlatformDebug.h"
#include "Window.h"
#include "Devices.h"
#include "SwapChain.h"

#pragma warning( push )
#pragma warning( disable: 26812 )

#if defined(VK_API_VERSION_1_3)
#define VULKAN_VERSION VK_API_VERSION_1_3
#else
#define VULKAN_VERSION VK_API_VERSION_1_1
#endif


Graphics* gGraphics = nullptr;

const bool gEnableValidation = true;
bool gInstanceValidationEnabled = false;

const char* VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";

VkInstance gVkInstance = VK_NULL_HANDLE;

VkDebugUtilsMessengerEXT gDebugMessenger = VK_NULL_HANDLE;

//debug
PFN_vkSetDebugUtilsObjectNameEXT gfDebugMarkerSetObjectName;
PFN_vkCmdBeginDebugUtilsLabelEXT gfDebugMarkerBegin;
PFN_vkCmdEndDebugUtilsLabelEXT gfDebugMarkerEnd;
PFN_vkCmdInsertDebugUtilsLabelEXT gfDebugMarkerInsert;


bool CheckVkResult(VkResult aResult) {
	switch (aResult) {
	case VK_SUCCESS:
		return true;
	default:
		ASSERT(false);
	}
	return false;
}

bool VkResultToBool(VkResult aResult) {
	return VK_SUCCESS == aResult;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	LOG::Log("VULKAN %s: \n\t %s\n", pCallbackData->pMessageIdName,
		pCallbackData->pMessage);

	if (pCallbackData->messageIdNumber != 0) {
		ASSERT("Vulkan Error");
	}

	return VK_FALSE;
}


bool Graphics::StartUp() {
	ASSERT(gGraphics == nullptr);
	gGraphics = this;

	VkResult result;
	{
		LOG::Log("Loading Vulkan sdk: ");
		uint32_t version = VK_HEADER_VERSION_COMPLETE;
		LOG::Log("Version: %i.%i.%i\n", VK_VERSION_MAJOR(version),
			VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
	}

	//instance data
	{
		//layer
		uint32_t layerCount = 0;
		result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		CheckVkResult(result);
		mInstanceLayers.resize(layerCount);
		result = vkEnumerateInstanceLayerProperties(&layerCount, mInstanceLayers.data());
		CheckVkResult(result);
		//extensions
		result = vkEnumerateInstanceExtensionProperties(nullptr, &layerCount, nullptr);
		CheckVkResult(result);
		mInstanceExtensions.resize(layerCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &layerCount, mInstanceExtensions.data());
		CheckVkResult(result);
	}

	CreateInstance();

	return false;
}

bool Graphics::Initalize() {
	ASSERT(gVkInstance != VK_NULL_HANDLE);

	mDevicesHandler = new Devices(mSurfaces[0]);
	mDevicesHandler->Setup();

	//vma
	{
		VmaAllocatorCreateInfo createInfo{};
		createInfo.vulkanApiVersion = VULKAN_VERSION;
		createInfo.device = GetDevice();
		createInfo.instance = gVkInstance;
		createInfo.physicalDevice = GetPhysicalDevice();
		createInfo.pAllocationCallbacks = GetAllocationCallback();

		vmaCreateAllocator(&createInfo, &mAllocator);
  	}

	mSwapChain = new SwapChain(mDevicesHandler->GetPrimaryDeviceData());
	mSwapChain->Setup();

	//commandpool/commandbuffers

	//frame syncing? merge into swapchain?

	//imgui

	//descriptor sets?

	return false;
}

bool Graphics::Destroy() {

	if (gDebugMessenger) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			gVkInstance, "vkDestroyDebugUtilsMessengerEXT");
		ASSERT(func != nullptr);
		if (func != nullptr) {
			func(gVkInstance, gDebugMessenger, GetAllocationCallback());
		}
	}

	if (gVkInstance) {
		vkDestroyInstance(gVkInstance, GetAllocationCallback());
		gVkInstance = VK_NULL_HANDLE;
	}

	return true;
}

void Graphics::AddWindow(Window* aWindow) {
	if (aWindow == nullptr) {
		return;
	}
	VkSurfaceKHR surface = (VkSurfaceKHR)aWindow->CreateSurface();
	ASSERT(surface != VK_NULL_HANDLE);
	if (surface) {
		mSurfaces.push_back(surface);
	}
}

const VkDevice Graphics::GetDevice() const {
	return mDevicesHandler->GetPrimaryDevice();
}

const VkPhysicalDevice Graphics::GetPhysicalDevice() const {
	return mDevicesHandler->GetPrimaryPhysicalDevice();
}

bool Graphics::CreateInstance() {
	VkResult result;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Graphics Playground";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = nullptr;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VULKAN_VERSION;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions;
	std::vector<const char*> layers;

	//glfw
	{
		uint32_t extensionCount = 0;
		const char** glfwExtentensions = Window::GetGLFWVulkanExtentensions(&extensionCount);
		extensions.assign(glfwExtentensions, glfwExtentensions + extensionCount);
	}

	//Debug validation
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (gEnableValidation) {
		gInstanceValidationEnabled = HasInstanceLayer(VK_LAYER_KHRONOS_validation);
		if (gInstanceValidationEnabled) {
			debugCreateInfo.sType =
				VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity =
				/*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = DebugCallback;
			debugCreateInfo.pNext = nullptr;

			layers.push_back(VK_LAYER_KHRONOS_validation);

			ASSERT(createInfo.pNext == nullptr);//make sure this is null
			createInfo.pNext = &debugCreateInfo;
		}
		//Debug Labels
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}


	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	createInfo.ppEnabledLayerNames = layers.data();


	result = vkCreateInstance(&createInfo, GetAllocationCallback(), &gVkInstance);
	CheckVkResult(result);

	if (gInstanceValidationEnabled) {
		auto getInstanceProcAddr = [](void* aFunc, const char* aName) {
			PFN_vkVoidFunction* func = (PFN_vkVoidFunction*)aFunc;
			*func = vkGetInstanceProcAddr(gVkInstance, aName);
			ASSERT(*func != nullptr);
			return *func != nullptr;
		};

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		if (getInstanceProcAddr(&vkCreateDebugUtilsMessengerEXT, "vkCreateDebugUtilsMessengerEXT")) {
			vkCreateDebugUtilsMessengerEXT(gVkInstance, &debugCreateInfo, GetAllocationCallback(),
				&gDebugMessenger);
		}

		getInstanceProcAddr(&gfDebugMarkerSetObjectName, "vkSetDebugUtilsObjectNameEXT");
		getInstanceProcAddr(&gfDebugMarkerBegin, "vkCmdBeginDebugUtilsLabelEXT");
		getInstanceProcAddr(&gfDebugMarkerEnd, "vkCmdEndDebugUtilsLabelEXT");
		getInstanceProcAddr(&gfDebugMarkerInsert, "vkCmdInsertDebugUtilsLabelEXT");
	}

	return VkResultToBool(result);
}

bool Graphics::HasInstanceExtension(const char* aExtension) const {
	for (int i = 0; i < mInstanceExtensions.size(); i++) {
		if (strcmp(aExtension, mInstanceExtensions[i].extensionName) == 0) {
			return true;
		}
	}
	return false;
}

bool Graphics::HasInstanceLayer(const char* aLayer) const {
	for (int i = 0; i < mInstanceLayers.size(); i++) {
		if (strcmp(aLayer, mInstanceLayers[i].layerName) == 0) {
			return true;
		}
	}
	return false;
}

#pragma warning( pop ) // 26812
