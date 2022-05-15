#include "Graphics.h"

#include <string>

#include "PlatformDebug.h"
#include "Window.h"
#include "Devices.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "Helpers.h"
#include "Pipeline.h"
#include "RenderPass.h"

#if defined(ENABLE_IMGUI)
#include "imgui.h"
#endif

#pragma warning( push )
#pragma warning( disable: 26812 )

#if defined(VK_API_VERSION_1_3)
#define VULKAN_VERSION VK_API_VERSION_1_3
#else
#define VULKAN_VERSION VK_API_VERSION_1_1
#endif

#if defined(ENABLE_XR)
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
XrInstance gXrInstance;
XrSystemId gXrSystemId;
std::vector<XrExtensionProperties> mXrInstanceExtensions;
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

bool getVkInstanceProcAddr(void* aFunc, const char* aName) {
	PFN_vkVoidFunction* func = (PFN_vkVoidFunction*)aFunc;
	*func = vkGetInstanceProcAddr(gVkInstance, aName);
	ASSERT(*func != nullptr);
	return *func != nullptr;
};

#if defined(ENABLE_XR)
XrDebugUtilsMessengerEXT gXrDebugMessenger = VK_NULL_HANDLE;

static XRAPI_ATTR XrBool32 XRAPI_CALL
XrDebugCallback(XrDebugUtilsMessageSeverityFlagsEXT              messageSeverity,
	XrDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const XrDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData) {

	const char* severityText[4] = { "VERBOSE",
		"INFO",
		"WARNING",
		"ERROR" };
	const char* typeText[4] = {
		"GENERAL",
		"VALIDATION",
		"PERFORMANCE",
		"CONFORMANCE" };
	LOG::Log("XR_DEBUG %s: (%s) (%s/%s)\n\t %s\n", callbackData->messageId, callbackData->functionName, severityText[messageSeverity], typeText[messageTypes],
		callbackData->message);

	//if (callbackData->messageId != 0) {
	//	ASSERT("XR Error");
	//}

	return VK_FALSE;
}

bool getXrInstanceProcAddr(void* aFunc, const char* aName) {
	PFN_xrVoidFunction* func = (PFN_xrVoidFunction*)aFunc;
	xrGetInstanceProcAddr(gXrInstance, aName, func);
	ASSERT(*func != nullptr);
	return *func != nullptr;
};
#endif

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
VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
	
	//XR
#if defined(ENABLE_XR)
	{
		LOG::Log("Loading OpenXR sdk: ");
		uint32_t version = XR_CURRENT_API_VERSION;
		LOG::Log("Version: %i.%i.%i\n", XR_VERSION_MAJOR(version),
			XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));

		XrResult xrResult;
		uint32_t extensionCount = 0;
		xrResult = xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);
		mXrInstanceExtensions.resize(extensionCount);

		for (int i = 0; i < extensionCount; i++) {
			mXrInstanceExtensions[i].type = XR_TYPE_EXTENSION_PROPERTIES;
		}

		xrResult = xrEnumerateInstanceExtensionProperties(NULL, extensionCount, &extensionCount, mXrInstanceExtensions.data());

		std::vector<const char*> extensions;// = { XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME };

		auto optionalXrExtension = [&](const char* aExtension) {
			for (int i = 0; i < extensionCount; i++) {
				if (strcmp(aExtension, mXrInstanceExtensions[i].extensionName) == 0) {
					extensions.push_back(aExtension);
					return true;
				}
			}
			return false;
		};

		auto requireXrExtension = [&](const char* aExtension) {
			if (optionalXrExtension(aExtension)) {
				return true;
			}
			ASSERT(false);
			return false;
		};

		//if (optionalXrExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME) == false) {
		//	requireXrExtension(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
		//}
		//requireXrExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
		requireXrExtension(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
		optionalXrExtension(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME);
		optionalXrExtension(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
		optionalXrExtension(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

		XrInstanceCreateInfo create = {};
		create.type = XR_TYPE_INSTANCE_CREATE_INFO;
		create.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
		create.applicationInfo.applicationVersion = 1;
		create.applicationInfo.applicationName;
		strcpy(create.applicationInfo.applicationName, "Graphics-Playground");
		create.enabledExtensionCount = extensions.size();
		create.enabledExtensionNames = extensions.data();
		xrCreateInstance(&create, &gXrInstance);

		XrDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.type =
			XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverities =
			//XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
		debugCreateInfo.userCallback = XrDebugCallback;
		debugCreateInfo.next = nullptr;

		PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXT;
		if (getXrInstanceProcAddr(&xrCreateDebugUtilsMessengerEXT, "xrCreateDebugUtilsMessengerEXT")) {
			xrCreateDebugUtilsMessengerEXT(gXrInstance, &debugCreateInfo, &gXrDebugMessenger);
		}

		XrSystemGetInfo systemGet = {};
		systemGet.type = XR_TYPE_SYSTEM_GET_INFO;
		systemGet.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		systemGet.next = NULL;

		xrResult = xrGetSystem(gXrInstance, &systemGet, &gXrSystemId);
	}
#endif

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
	mDevicesHandler->CreateCommandPools();

	mSwapchain = new Swapchain(mDevicesHandler->GetPrimaryDeviceData());
	mSwapchain->Setup();

	mDevicesHandler->CreateCommandBuffers(mSwapchain->GetNumBuffers());

	//vma
	{
		VmaAllocatorCreateInfo createInfo{};
		createInfo.vulkanApiVersion = VULKAN_VERSION;
		createInfo.device = GetVkDevice();
		createInfo.instance = gVkInstance;
		createInfo.physicalDevice = GetVkPhysicalDevice();
		createInfo.pAllocationCallbacks = GetAllocationCallback();

		vmaCreateAllocator(&createInfo, &mAllocator);
	}

	//imgui
#if defined(ENABLE_IMGUI)
	CreateImGui();
#endif

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

	vmaDestroyAllocator(mAllocator);
	mAllocator = VK_NULL_HANDLE;

	mSwapchain->Destroy();

	mDevicesHandler->Destroy();

#if defined(ENABLE_XR)
	if (gXrInstance) {
		xrDestroyInstance(gXrInstance);
		gXrInstance = XR_NULL_HANDLE;
	}
#endif

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

void Graphics::StartNewFrame() {
	mFrameCounter++;
	uint32_t index = mSwapchain->GetNextImage();

	VkCommandBuffer graphics = mDevicesHandler->GetGraphicsCB(index);
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(graphics, &info);
}

void Graphics::EndFrame() {
	VkCommandBuffer graphics = mDevicesHandler->GetGraphicsCB(mSwapchain->GetImageIndex());
	vkEndCommandBuffer(graphics);

	mSwapchain->SubmitQueue(mDevicesHandler->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, { graphics });

	mSwapchain->PresentImage();
}

const uint32_t Graphics::GetCurrentImageIndex() const {
	return mSwapchain->GetImageIndex();
}

VkCommandBuffer Graphics::GetCurrentGraphicsCommandBuffer() const {
	return GetMainDevice()->GetGraphicsCB(GetCurrentImageIndex());
}

OneTimeCommandBuffer Graphics::AllocateGraphicsCommandBuffer() {
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = mDevicesHandler->GetGraphicsPool();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer buffer;
	vkAllocateCommandBuffers(GetVkDevice(), &allocateInfo, &buffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(buffer, &beginInfo);

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence;
	vkCreateFence(GetVkDevice(), &fenceInfo, GetAllocationCallback(), &fence);

	return { buffer, fence };
}

void Graphics::EndGraphicsCommandBuffer(OneTimeCommandBuffer aBuffer) {
	vkEndCommandBuffer(aBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pCommandBuffers = &aBuffer.mBuffer;

	vkQueueSubmit(mDevicesHandler->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, 1, &submitInfo, aBuffer.mFence);
	vkWaitForFences(GetVkDevice(), 1, &aBuffer.mFence, true, UINT64_MAX);

	vkDestroyFence(GetVkDevice(), aBuffer.mFence, GetAllocationCallback());
	vkFreeCommandBuffers(GetVkDevice(), mDevicesHandler->GetGraphicsPool(), 1, &aBuffer.mBuffer);
}

const VkDevice Graphics::GetVkDevice() const {
	return mDevicesHandler->GetPrimaryDevice();
}

const VkPhysicalDevice Graphics::GetVkPhysicalDevice() const {
	return mDevicesHandler->GetPrimaryPhysicalDevice();
}

const Devices* Graphics::GetMainDevice() const {
	return mDevicesHandler;
}

const Swapchain* Graphics::GetMainSwapchain() const {
	return mSwapchain;
}

const VkFormat Graphics::GetMainFormat() const {
	return GetMainSwapchain()->GetColorFormat();
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

	std::vector<std::string> extensionsTemp;
	std::vector<const char*> extensions;
	std::vector<const char*> layers;

	//glfw
	{
		uint32_t extensionCount = 0;
		const char** glfwExtentensions = Window::GetGLFWVulkanExtentensions(&extensionCount);
		extensions.assign(glfwExtentensions, glfwExtentensions + extensionCount);
	}

#if defined(ENABLE_XR)
	//openXR
	{
		PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensionsKHR;
		getXrInstanceProcAddr(&xrGetVulkanInstanceExtensionsKHR, "xrGetVulkanInstanceExtensionsKHR");

		uint32_t extensionBufferSize = 0;
		XrResult xrResult;
		xrResult = xrGetVulkanInstanceExtensionsKHR(gXrInstance, gXrSystemId, 0, &extensionBufferSize, NULL);
		char* xrExtensions = new char[extensionBufferSize];
		xrResult = xrGetVulkanInstanceExtensionsKHR(gXrInstance, gXrSystemId, extensionBufferSize, &extensionBufferSize, xrExtensions);

		int lastPoint = 0;
		for (int i = 0; i < extensionBufferSize; i++) {
			if (xrExtensions[i] == ' ') {
				xrExtensions[i] = '\0';
				extensionsTemp.push_back(&xrExtensions[lastPoint]);
				lastPoint = i + 1;
			}
		}
		extensionsTemp.push_back(&xrExtensions[lastPoint]);
		delete[] xrExtensions;
	}
#endif

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
			debugCreateInfo.pfnUserCallback = VkDebugCallback;
			debugCreateInfo.pNext = nullptr;

			layers.push_back(VK_LAYER_KHRONOS_validation);

			ASSERT(createInfo.pNext == nullptr);//make sure this is null
			createInfo.pNext = &debugCreateInfo;
		}
		//Debug Labels
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	//copy over any temporary strings
	for (int i = 0; i < extensionsTemp.size(); i++) {
		extensions.push_back(extensionsTemp[i].c_str());
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	createInfo.ppEnabledLayerNames = layers.data();


	result = vkCreateInstance(&createInfo, GetAllocationCallback(), &gVkInstance);
	CheckVkResult(result);

	if (gInstanceValidationEnabled) {
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		if (getVkInstanceProcAddr(&vkCreateDebugUtilsMessengerEXT, "vkCreateDebugUtilsMessengerEXT")) {
			vkCreateDebugUtilsMessengerEXT(gVkInstance, &debugCreateInfo, GetAllocationCallback(),
				&gDebugMessenger);
		}

		getVkInstanceProcAddr(&gfDebugMarkerSetObjectName, "vkSetDebugUtilsObjectNameEXT");
		getVkInstanceProcAddr(&gfDebugMarkerBegin, "vkCmdBeginDebugUtilsLabelEXT");
		getVkInstanceProcAddr(&gfDebugMarkerEnd, "vkCmdEndDebugUtilsLabelEXT");
		getVkInstanceProcAddr(&gfDebugMarkerInsert, "vkCmdInsertDebugUtilsLabelEXT");
	}

	return VkResultToBool(result);
}

#if defined(ENABLE_IMGUI)
bool Graphics::CreateImGui() {
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	unsigned char* fontData;
	int fontWidth;
	int fontHeight;
	int fontBytes;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &fontWidth, &fontHeight, &fontBytes);
	const int bufferSize = fontWidth * fontHeight * sizeof(unsigned char) * fontBytes;
	ImageSize size(fontWidth, fontHeight);

	Buffer fontBuffer;
	fontBuffer.CreateFromData(bufferSize, fontData);

	mImGuiFontImage.CreateFromBuffer(fontBuffer, VK_FORMAT_R8G8B8A8_UNORM, size);

	fontBuffer.Destroy();

	Pipeline imguiPipeline;
	imguiPipeline.AddShader(std::string(WORK_DIR_REL) + "WorkDir/Shaders/imgui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	imguiPipeline.AddShader(std::string(WORK_DIR_REL) + "WorkDir/Shaders/imgui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

	RenderPass test;
	test.Create();

	imguiPipeline.Create(test.GetRenderPass());

	return true;
}
#endif

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
