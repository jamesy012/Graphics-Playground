#include "Graphics.h"

#include <string>

#include <glm/glm.hpp>

#include "Buffer.h"
#include "Devices.h"
#include "Helpers.h"
#include "Pipeline.h"
#include "PlatformDebug.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "Engine/Window.h"

#if defined(ENABLE_IMGUI)
#	include "ImGuiGraphics.h"

ImGuiGraphics gImGuiGraphics;
#endif

#if defined(ENABLE_XR)
#	include "VRGraphics.h"

VRGraphics gVrGraphics;
#endif

#pragma warning(push)
#pragma warning(disable : 26812)

#if defined(VK_API_VERSION_1_3)
#	define VULKAN_VERSION VK_API_VERSION_1_3
#else
#	define VULKAN_VERSION VK_API_VERSION_1_1
#endif

VulkanGraphics* gGraphics = nullptr;

const bool gEnableValidation	= true;
bool gInstanceValidationEnabled = false;

const char* VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";

VkInstance gVkInstance = VK_NULL_HANDLE;

VkDebugUtilsMessengerEXT gDebugMessenger = VK_NULL_HANDLE;

// debug
PFN_vkSetDebugUtilsObjectNameEXT gfDebugMarkerSetObjectName;
PFN_vkCmdBeginDebugUtilsLabelEXT gfDebugMarkerBegin;
PFN_vkCmdEndDebugUtilsLabelEXT gfDebugMarkerEnd;
PFN_vkCmdInsertDebugUtilsLabelEXT gfDebugMarkerInsert;

bool getVkInstanceProcAddr(void* aFunc, const char* aName) {
	PFN_vkVoidFunction* func = (PFN_vkVoidFunction*)aFunc;
	*func					 = vkGetInstanceProcAddr(gVkInstance, aName);
	ASSERT(*func != nullptr);
	return *func != nullptr;
};

bool CheckVkResult(VkResult aResult) {
	switch(aResult) {
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

static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													  VkDebugUtilsMessageTypeFlagsEXT messageType,
													  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	LOGGER::Formated("VULKAN {}: \n\t{}\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);

	if(pCallbackData->messageIdNumber != 0) {
		ASSERT("Vulkan Error");
	}

	return VK_FALSE;
}

// template<typename T>
// void SetVkName(VkObjectType aType, T aObject, const char* aName) {
void SetVkName(VkObjectType aType, uint64_t aObject, const char* aName) {
	if(gfDebugMarkerSetObjectName && gGraphics && gGraphics->GetVkDevice()) {
		VkDebugUtilsObjectNameInfoEXT info = {};
		info.sType						   = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		info.objectType					   = aType;
		info.objectHandle				   = aObject;
		info.pObjectName				   = aName;
		// vkSetDebugUtilsObjectNameEXT(gGraphics->GetVkDevice(), &info);
		gfDebugMarkerSetObjectName(gGraphics->GetVkDevice(), &info);
	}
}

bool VulkanGraphics::Startup() {
	ASSERT(gGraphics == nullptr);
	gGraphics = this;
	LOGGER::Log("Starting Graphics\n");

	VkResult result;
	{
		LOGGER::Log("Loading Vulkan sdk: ");
		uint32_t version = VK_HEADER_VERSION_COMPLETE;
		LOGGER::Formated("Version: {0}.{1}.{2}\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
	}

	// XR
#if defined(ENABLE_XR)
	gVrGraphics.Startup();
#endif

	// instance data
	{
		// layer
		uint32_t layerCount = 0;
		result				= vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		CheckVkResult(result);
		mInstanceLayers.resize(layerCount);
		result = vkEnumerateInstanceLayerProperties(&layerCount, mInstanceLayers.data());
		CheckVkResult(result);
		// extensions
		result = vkEnumerateInstanceExtensionProperties(nullptr, &layerCount, nullptr);
		CheckVkResult(result);
		mInstanceExtensions.resize(layerCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &layerCount, mInstanceExtensions.data());
		CheckVkResult(result);
	}

	CreateInstance();

	return false;
}

bool VulkanGraphics::Initalize() {
	ASSERT(gVkInstance != VK_NULL_HANDLE);
	LOGGER::Log("Initalize Graphics\n");

	mDevicesHandler = new Devices((VkSurfaceKHR)mSurfaces[0]->GetSurface());
	mDevicesHandler->Setup();
	mDevicesHandler->CreateCommandPools();

	mSwapchain = new Swapchain(mDevicesHandler->GetPrimaryDeviceData());
	int width, height;
	mSurfaces[0]->GetSize(&width, &height);
	mSwapchain->Setup(ImageSize(width, height));

#if defined(ENABLE_XR)
	//needs the device setup
	gVrGraphics.Initalize();
#endif

	mDevicesHandler->CreateCommandBuffers(mSwapchain->GetNumBuffers());

	// vma
	{
		VmaAllocatorCreateInfo createInfo {};
		createInfo.vulkanApiVersion		= VULKAN_VERSION;
		createInfo.device				= GetVkDevice();
		createInfo.instance				= gVkInstance;
		createInfo.physicalDevice		= GetVkPhysicalDevice();
		createInfo.pAllocationCallbacks = GetAllocationCallback();

		vmaCreateAllocator(&createInfo, &mAllocator);
	}

	{
		std::vector<VkClearValue> clear(1);
		clear[0].color.float32[0] = 0.0f;
		mRenderPass.SetClearColors(clear);

		mRenderPass.AddColorAttachment(GetSwapchainFormat(), VK_ATTACHMENT_LOAD_OP_LOAD);
		mRenderPass.Create("Present Renderpass");
		mFramebuffer[0].AddImage(&mSwapchain->GetImage(0));
		mFramebuffer[0].Create(mRenderPass, "Swapchain Framebuffer 0");
		mFramebuffer[1].AddImage(&mSwapchain->GetImage(1));
		mFramebuffer[1].Create(mRenderPass, "Swapchain Framebuffer 1");
		mFramebuffer[2].AddImage(&mSwapchain->GetImage(2));
		mFramebuffer[2].Create(mRenderPass, "Swapchain Framebuffer 2");
#if defined(ENABLE_XR)
		mXrRenderPass.SetClearColors(clear);
		mXrRenderPass.AddColorAttachment(GetXRSwapchainFormat(), VK_ATTACHMENT_LOAD_OP_LOAD);
		mXrRenderPass.Create("Present Renderpass");
		for(int eye = 0; eye < 2; eye++) {
			for(int sc = 0; sc < 3; sc++) {
				mXrFramebuffer[eye][sc].AddImage(&gVrGraphics.GetImage(eye, sc));
				mXrFramebuffer[eye][sc].Create(mXrRenderPass, "xr Swapchain Framebuffer ");
			}
		}
		//mXrFramebuffer[0][1].Create(gVrGraphics.GetImage(0, 1), mRenderPass, "xr Swapchain Framebuffer 0/1");
		//mXrFramebuffer[0][2].Create(gVrGraphics.GetImage(0, 2), mRenderPass, "xr Swapchain Framebuffer 0/2");
		//mXrFramebuffer[1][0].Create(gVrGraphics.GetImage(1, 0), mRenderPass, "xr Swapchain Framebuffer 1/0");
		//mXrFramebuffer[1][1].Create(gVrGraphics.GetImage(1, 1), mRenderPass, "xr Swapchain Framebuffer 1/1");
		//mXrFramebuffer[1][2].Create(gVrGraphics.GetImage(1, 2), mRenderPass, "xr Swapchain Framebuffer 1/2");
#endif
	}

	{
		VkDescriptorPoolSize pools[] = {
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50},
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType					  = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.maxSets					  = 50;
		descriptorPoolInfo.pPoolSizes				  = pools;
		descriptorPoolInfo.poolSizeCount			  = sizeof(pools) / sizeof(VkDescriptorPoolSize);
		vkCreateDescriptorPool(GetVkDevice(), &descriptorPoolInfo, GetAllocationCallback(), &mDescriptorPool);
		SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, mDescriptorPool, "Default Descriptor Pool");
	}

	{
		VkSamplerCreateInfo samplerInfo		= {};
		samplerInfo.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter				= VK_FILTER_LINEAR;
		samplerInfo.minFilter				= VK_FILTER_LINEAR;
		samplerInfo.addressModeU			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable		= VK_TRUE;
		samplerInfo.maxAnisotropy			= GetMainDevice()->GetPrimaryDeviceData().mDeviceProperties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor				= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable			= VK_FALSE;
		samplerInfo.compareOp				= VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias				= 0.0f;
		samplerInfo.minLod					= 0.0f;
		samplerInfo.maxLod					= 0.0f;
		vkCreateSampler(GetVkDevice(), &samplerInfo, GetAllocationCallback(), &mSampler);
		SetVkName(VK_OBJECT_TYPE_SAMPLER, mSampler, "Default Repeat Sampler");
	}

	// imgui
#if defined(ENABLE_IMGUI)
	gImGuiGraphics.Create(mSurfaces[0]->GetWindow(), mRenderPass);
#endif

	return true;
}

bool VulkanGraphics::Destroy() {

	if(gDebugMessenger) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(gVkInstance, "vkDestroyDebugUtilsMessengerEXT");
		ASSERT(func != nullptr);
		if(func != nullptr) {
			func(gVkInstance, gDebugMessenger, GetAllocationCallback());
		}
	}

	vkDestroySampler(GetVkDevice(), mSampler, GetAllocationCallback());
	// vkFreeDescriptorSets(GetVkDevice(), gDescriptorPool, 1, &gImGuiFontSet);
	vkDestroyDescriptorPool(GetVkDevice(), mDescriptorPool, GetAllocationCallback());

#if defined(ENABLE_IMGUI)
	gImGuiGraphics.Destroy();
#endif

#if defined(ENABLE_XR)
	mXrFramebuffer[0][0].Destroy();
	mXrFramebuffer[0][1].Destroy();
	mXrFramebuffer[0][2].Destroy();
	mXrFramebuffer[1][0].Destroy();
	mXrFramebuffer[1][1].Destroy();
	mXrFramebuffer[1][2].Destroy();
	mXrRenderPass.Destroy();
#endif

	mFramebuffer[0].Destroy();
	mFramebuffer[1].Destroy();
	mFramebuffer[2].Destroy();
	mRenderPass.Destroy();

	vmaDestroyAllocator(mAllocator);
	mAllocator = VK_NULL_HANDLE;

	mSwapchain->Destroy();

#if defined(ENABLE_XR)
	gVrGraphics.Destroy();
	LOGGER::Log("~~~~~~~~~~~~~ XR Vulkan doesnt delete a command pool\n?");
#endif

	mDevicesHandler->Destroy();

	for(int i = 0; i < mSurfaces.size(); i++) {
		mSurfaces[i]->DestroySurface();
	}

	if(gVkInstance) {
		vkDestroyInstance(gVkInstance, GetAllocationCallback());
		gVkInstance = VK_NULL_HANDLE;
	}

	return true;
}

void VulkanGraphics::AddWindow(Window* aWindow) {
	if(aWindow == nullptr) {
		return;
	}
	VkSurfaceKHR surface = (VkSurfaceKHR)aWindow->CreateSurface();
	ASSERT(surface != VK_NULL_HANDLE);
	if(surface) {
		mSurfaces.push_back(aWindow);
	}
}

void VulkanGraphics::StartNewFrame() {
#if defined(ENABLE_IMGUI)
	gImGuiGraphics.StartNewFrame();
#endif

	mFrameCounter++;
	uint32_t index = mSwapchain->GetNextImage();

	VkCommandBuffer graphics	  = mDevicesHandler->GetGraphicsCB(index);
	VkCommandBufferBeginInfo info = {};
	info.sType					  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(graphics, &info);

#if defined(ENABLE_XR)
	gVrGraphics.FrameBegin(graphics);
#endif

	//reset viewport and scissor
	VkViewport viewport = {};
	viewport.width		= gGraphics->GetMainSwapchain()->GetSize().width;
	viewport.height		= gGraphics->GetMainSwapchain()->GetSize().height;
	viewport.maxDepth	= 1.0f;
	vkCmdSetViewport(graphics, 0, 1, &viewport);

	VkRect2D scissorRect;
	scissorRect.offset.x	  = 0;
	scissorRect.offset.y	  = 0;
	scissorRect.extent.width  = viewport.width;
	scissorRect.extent.height = viewport.height;
	vkCmdSetScissor(graphics, 0, 1, &scissorRect);

	mSwapchain->GetImage(GetCurrentImageIndex())
		.SetImageLayout(graphics,
						VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void VulkanGraphics::EndFrame() {
	VkCommandBuffer graphics = mDevicesHandler->GetGraphicsCB(mSwapchain->GetImageIndex());

#if defined(ENABLE_IMGUI)
	gImGuiGraphics.RenderImGui(graphics, mRenderPass, mFramebuffer[GetCurrentImageIndex()]);
#endif

	mSwapchain->GetImage(GetCurrentImageIndex())
		.SetImageLayout(graphics,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	vkEndCommandBuffer(graphics);

	mSwapchain->SubmitQueue(mDevicesHandler->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, {graphics});

#if defined(ENABLE_XR)
	gVrGraphics.FrameEnd();
#endif

	mSwapchain->PresentImage();
}

const uint32_t VulkanGraphics::GetCurrentImageIndex() const {
	return mSwapchain->GetImageIndex();
}

VkCommandBuffer VulkanGraphics::GetCurrentGraphicsCommandBuffer() const {
	return GetMainDevice()->GetGraphicsCB(GetCurrentImageIndex());
}

#if defined(ENABLE_XR)
const Framebuffer& VulkanGraphics::GetCurrentXrFrameBuffer(uint8_t aEye) const {
	return mXrFramebuffer[aEye][gVrGraphics.GetCurrentImageIndex(aEye)];
}
#endif

OneTimeCommandBuffer VulkanGraphics::AllocateGraphicsCommandBuffer() {
	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType						 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool				 = mDevicesHandler->GetGraphicsPool();
	allocateInfo.level						 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount			 = 1;

	VkCommandBuffer buffer;
	vkAllocateCommandBuffers(GetVkDevice(), &allocateInfo, &buffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType					   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags					   = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(buffer, &beginInfo);

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType				= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence;
	vkCreateFence(GetVkDevice(), &fenceInfo, GetAllocationCallback(), &fence);

	SetVkName(VK_OBJECT_TYPE_COMMAND_BUFFER, buffer, "Graphics CommandBuffer");
	SetVkName(VK_OBJECT_TYPE_FENCE, fence, "Graphics CommandBuffer Fence");

	return {buffer, fence};
}

void VulkanGraphics::EndGraphicsCommandBuffer(OneTimeCommandBuffer aBuffer) {
	vkEndCommandBuffer(aBuffer);

	VkSubmitInfo submitInfo		  = {};
	submitInfo.sType			  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pCommandBuffers	  = &aBuffer.mBuffer;
	submitInfo.commandBufferCount = 1;

	vkQueueSubmit(mDevicesHandler->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, 1, &submitInfo, aBuffer.mFence);
	vkWaitForFences(GetVkDevice(), 1, &aBuffer.mFence, true, UINT64_MAX);

	vkDestroyFence(GetVkDevice(), aBuffer.mFence, GetAllocationCallback());
	vkFreeCommandBuffers(GetVkDevice(), mDevicesHandler->GetGraphicsPool(), 1, &aBuffer.mBuffer);
}

const VkInstance VulkanGraphics::GetVkInstance() const {
	return gVkInstance;
}

const VkDevice VulkanGraphics::GetVkDevice() const {
	return mDevicesHandler->GetPrimaryDevice();
}

const VkPhysicalDevice VulkanGraphics::GetVkPhysicalDevice() const {
	return mDevicesHandler->GetPrimaryPhysicalDevice();
}

const Devices* VulkanGraphics::GetMainDevice() const {
	return mDevicesHandler;
}

const Swapchain* VulkanGraphics::GetMainSwapchain() const {
	return mSwapchain;
}

#if defined(ENABLE_XR)
const VRGraphics* VulkanGraphics::GetVrGraphics() const {
	return &gVrGraphics;
}
#endif

const bool VulkanGraphics::IsFormatDepth(VkFormat aFormat) {
	if(aFormat >= VK_FORMAT_D16_UNORM && aFormat <= VK_FORMAT_D32_SFLOAT_S8_UINT) {
		return true;
	}
	return false;
}

//static
const VkFormat VulkanGraphics::GetDeafultColorFormat() {
	//todo check that this format is supported
	return VK_FORMAT_R8G8B8A8_UNORM;
}

//static
const VkFormat VulkanGraphics::GetDeafultDepthFormat() {
	//todo check that this format is supported
	return VK_FORMAT_D32_SFLOAT;
}

const VkFormat VulkanGraphics::GetSwapchainFormat() const {
	return GetMainSwapchain()->GetColorFormat();
}

#if defined(ENABLE_XR)
const VkFormat VulkanGraphics::GetXRSwapchainFormat() const {
	return gVrGraphics.GetSwapchainFormat();
}
#endif

bool VulkanGraphics::CreateInstance() {
	VkResult result;

	VkApplicationInfo appInfo  = {};
	appInfo.sType			   = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "Graphics Playground";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName		   = nullptr;
	appInfo.engineVersion	   = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion		   = VULKAN_VERSION;

	VkInstanceCreateInfo createInfo {};
	createInfo.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<std::string> extensionsTemp;
	std::vector<const char*> extensions;
	std::vector<const char*> layers;

	// glfw
	{
		uint32_t extensionCount		   = 0;
		const char** glfwExtentensions = Window::GetGLFWVulkanExtentensions(&extensionCount);
		extensions.assign(glfwExtentensions, glfwExtentensions + extensionCount);
	}

	//not needed?
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#if defined(ENABLE_XR)
	std::vector<std::string> xrExtensions = gVrGraphics.GetVulkanInstanceExtensions();
	extensionsTemp.insert(extensionsTemp.end(), xrExtensions.begin(), xrExtensions.end());
#endif

	// Debug validation
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
	if(gEnableValidation) {
		gInstanceValidationEnabled = HasInstanceLayer(VK_LAYER_KHRONOS_validation);
		if(gInstanceValidationEnabled) {
			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity =
				/*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = VkDebugCallback;
			debugCreateInfo.pNext			= nullptr;

			layers.push_back(VK_LAYER_KHRONOS_validation);

			ASSERT(createInfo.pNext == nullptr); // make sure this is null
			createInfo.pNext = &debugCreateInfo;
		}
		// Debug Labels
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// copy over any temporary strings
	for(int i = 0; i < extensionsTemp.size(); i++) {
		extensions.push_back(extensionsTemp[i].c_str());
	}

	createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount	   = static_cast<uint32_t>(layers.size());
	createInfo.ppEnabledLayerNames	   = layers.data();

	result = vkCreateInstance(&createInfo, GetAllocationCallback(), &gVkInstance);
	CheckVkResult(result);

	if(gInstanceValidationEnabled) {
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		if(getVkInstanceProcAddr(&vkCreateDebugUtilsMessengerEXT, "vkCreateDebugUtilsMessengerEXT")) {
			vkCreateDebugUtilsMessengerEXT(gVkInstance, &debugCreateInfo, GetAllocationCallback(), &gDebugMessenger);
		}

		getVkInstanceProcAddr(&gfDebugMarkerSetObjectName, "vkSetDebugUtilsObjectNameEXT");
		getVkInstanceProcAddr(&gfDebugMarkerBegin, "vkCmdBeginDebugUtilsLabelEXT");
		getVkInstanceProcAddr(&gfDebugMarkerEnd, "vkCmdEndDebugUtilsLabelEXT");
		getVkInstanceProcAddr(&gfDebugMarkerInsert, "vkCmdInsertDebugUtilsLabelEXT");
	}

	return VkResultToBool(result);
}

bool VulkanGraphics::HasInstanceExtension(const char* aExtension) const {
	for(int i = 0; i < mInstanceExtensions.size(); i++) {
		if(strcmp(aExtension, mInstanceExtensions[i].extensionName) == 0) {
			return true;
		}
	}
	return false;
}

bool VulkanGraphics::HasInstanceLayer(const char* aLayer) const {
	for(int i = 0; i < mInstanceLayers.size(); i++) {
		if(strcmp(aLayer, mInstanceLayers[i].layerName) == 0) {
			return true;
		}
	}
	return false;
}

#pragma warning(pop) // 26812