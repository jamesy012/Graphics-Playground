#include "Graphics.h"

#include <string.h>
#include <map>

#include <glm/glm.hpp>

#include "Buffer.h"
#include "Devices.h"
#include "Helpers.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "Engine/Window.h"
#include "Image.h"
#include "MaterialManager.h"

#if defined(ENABLE_IMGUI)
#	include "ImGuiGraphics.h"
#endif

#if defined(ENABLE_XR)
#	include "VRGraphics.h"
#endif

#pragma warning(push)
#pragma warning(disable : 26812)

#if defined(VK_API_VERSION_1_3)
#	define VULKAN_VERSION VK_API_VERSION_1_3
#else
#	define VULKAN_VERSION VK_API_VERSION_1_1
#endif

VulkanGraphics* gGraphics = nullptr;

const bool gEnableValidation = true;
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
	*func = vkGetInstanceProcAddr(gVkInstance, aName);
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

std::map<uint32_t, bool> gVulkanValidationToggle;

static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													  VkDebugUtilsMessageTypeFlagsEXT messageType,
													  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	ZoneScopedC(0xFF0000);
	TracyMessage(pCallbackData->pMessage, strlen(pCallbackData->pMessage));

	bool shouldLog = true;

	auto toggleValue = gVulkanValidationToggle.find(pCallbackData->messageIdNumber);
	if(toggleValue != gVulkanValidationToggle.end()) {
		shouldLog = toggleValue->second;
	}

	if(shouldLog) {
		LOGGER::Formated("VULKAN {}: \n\t{}\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);

		if(pCallbackData->messageIdNumber != 0) {
			ASSERT(false); //"Vulkan Error");
		}
	}

	return VK_FALSE;
}

void VulkanValidationMessage(int32_t aMessageId, bool aEnabled) {
	gVulkanValidationToggle[aMessageId] = aEnabled;
}

// template<typename T>
// void SetVkName(VkObjectType aType, T aObject, const char* aName) {
void SetVkName(VkObjectType aType, uint64_t aObject, const char* aName) {
	if(gfDebugMarkerSetObjectName && gGraphics && gGraphics->GetVkDevice()) {
		VkDebugUtilsObjectNameInfoEXT info = {};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		info.objectType = aType;
		info.objectHandle = aObject;
		info.pObjectName = aName;
		// vkSetDebugUtilsObjectNameEXT(gGraphics->GetVkDevice(), &info);
		gfDebugMarkerSetObjectName(gGraphics->GetVkDevice(), &info);
	}
}

bool VulkanGraphics::Startup() {
	ZoneScoped;
	ASSERT(gGraphics == nullptr);
	gGraphics = this;
	LOGGER::Log("Starting Graphics\n");

	VkResult result;
	{
		LOGGER::Log("Loading Vulkan: \n");
		uint32_t sdkVersion = VK_HEADER_VERSION_COMPLETE;
		uint32_t version = VULKAN_VERSION;
		LOGGER::Formated("\tSDK: {0}.{1}.{2}\n", VK_VERSION_MAJOR(sdkVersion), VK_VERSION_MINOR(sdkVersion), VK_VERSION_PATCH(sdkVersion));
		LOGGER::Formated("\tVersion: {0}.{1}.{2}\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
	}

	// XR
#if defined(ENABLE_XR)
	//sets up gVrGraphics as this, will delete in shutdown
	VRGraphics* vrGraphics = new VRGraphics();
	vrGraphics->Startup();
#endif

	// instance data
	{
		// layer
		uint32_t layerCount = 0;
		result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		CheckVkResult(result);
		mInstanceLayers.resize(layerCount);
		result = vkEnumerateInstanceLayerProperties(&layerCount, mInstanceLayers.data());
		CheckVkResult(result);
		{
			VkLayerProperties nullLayer;
			memset(nullLayer.description, 0, sizeof(char));
			memset(nullLayer.layerName, 0, sizeof(char));
			nullLayer.implementationVersion = -1;
			nullLayer.implementationVersion = -1;
			mInstanceLayers.insert(mInstanceLayers.begin(), nullLayer);
		}
		mLayerExtensions.resize(mInstanceLayers.size());
		for(int i = 0; i < mInstanceLayers.size(); i++) {
			VkLayerProperties& layer = mInstanceLayers[i];
			std::vector<VkExtensionProperties>& extensions = mLayerExtensions[i];
			// extensions
			uint32_t extensionCount = 0;
			result = vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, nullptr);
			CheckVkResult(result);
			extensions.resize(extensionCount);
			result = vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, extensions.data());
			CheckVkResult(result);
		}
	}

	CreateInstance();

	return false;
}

bool VulkanGraphics::Initalize() {
	ZoneScoped;
	ASSERT(gVkInstance != VK_NULL_HANDLE);
	LOGGER::Log("Initalize Graphics\n");

	mDevicesHandler = new Devices((VkSurfaceKHR)mSurfaces[0]->GetSurface());
	mDevicesHandler->Setup();
	mDevicesHandler->CreateCommandPools();

	mSwapchain = new Swapchain(mDevicesHandler->GetPrimaryDeviceData());
	mSwapchain->Setup();

#if defined(ENABLE_XR)
	//needs the device setup
	gVrGraphics->Initalize();
#endif

	mDevicesHandler->CreateCommandBuffers(mSwapchain->GetNumBuffers());

	// vma
	{
		VmaAllocatorCreateInfo createInfo {};
		createInfo.vulkanApiVersion = VULKAN_VERSION;
		createInfo.device = GetVkDevice();
		createInfo.instance = gVkInstance;
		createInfo.physicalDevice = GetVkPhysicalDevice();
		createInfo.pAllocationCallbacks = GetAllocationCallback();

		vmaCreateAllocator(&createInfo, &mAllocator);
	}

	{
		std::vector<VkClearValue> clear(1);
		clear[0].color.float32[0] = 0.0f;
		mRenderPass.SetClearColors(clear);

		mRenderPass.AddColorAttachment(GetSwapchainFormat(), VK_ATTACHMENT_LOAD_OP_LOAD);
		mRenderPass.Create("Present Renderpass");

		auto CreateFramebuffers = [=]() {
			for(int i = 0; i < 3; i++) {
				if(mFramebuffer[i].GetFramebuffer() != VK_NULL_HANDLE) {
					mFramebuffer[i].Destroy();
				}
				mFramebuffer[i].AddImage(&mSwapchain->GetImage(i));
				mFramebuffer[i].Create(mRenderPass, "Swapchain Framebuffer");
			}
		};
		mResizeMessage.push_back(CreateFramebuffers);
		CreateFramebuffers();
#if defined(ENABLE_XR)
		mXrRenderPass.SetClearColors(clear);
		mXrRenderPass.AddColorAttachment(GetXRSwapchainFormat(), VK_ATTACHMENT_LOAD_OP_LOAD);
		mXrRenderPass.Create("Present Renderpass");
		for(int eye = 0; eye < 2; eye++) {
			for(int sc = 0; sc < 3; sc++) {
				mXrFramebuffer[eye][sc].AddImage(&gVrGraphics->GetImage(eye, sc));
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

	//global descriptor pool
	//todo look into a better solution?
	{
		static const uint32_t resourceSizes = 16536 * 2; //doubleing as temp
		LOGGER::Log("\n!!!!!!!!!!!\n\tdoubled global texture resource cap, need to revise\n!!!!!!!!!!!\n\n");
		//bistro model has ~2000 models
		//in PER_DRAW mode we it needs 4 elements which means 4*2000, 8000 resources used
		//PER_DRAW also keeps the last two frames of resources allocated which means it used 16000 resources
		//so it fails to allocate a new set of textures on the 3rd frame
		//will likely need to do it by material instead of mesh (71 materials per model drawn)
		//or reuse allocated sets if the textures match (probably the better but slower? solution)
		VkDescriptorPoolSize pools[] = {
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resourceSizes},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, resourceSizes},
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptorPoolInfo.pPoolSizes = pools;
		descriptorPoolInfo.poolSizeCount = sizeof(pools) / sizeof(VkDescriptorPoolSize);
		descriptorPoolInfo.maxSets = resourceSizes * descriptorPoolInfo.poolSizeCount;
		vkCreateDescriptorPool(GetVkDevice(), &descriptorPoolInfo, GetAllocationCallback(), &mDescriptorPool);
		SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, mDescriptorPool, "Default Descriptor Pool");
	}

	ASSERT(!mMaterialManager);
	mMaterialManager = new MaterialManager();
	mMaterialManager->Initalize();

	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = GetMainDevice()->GetPrimaryDeviceData().mDeviceProperties.properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		vkCreateSampler(GetVkDevice(), &samplerInfo, GetAllocationCallback(), &mSampler);
		SetVkName(VK_OBJECT_TYPE_SAMPLER, mSampler, "Default Repeat Sampler");
	}

	{
		CONSTANTS::IMAGE::gWhite = new Image();
		const unsigned char whiteData[] = {255, 255, 255, 255};
		CONSTANTS::IMAGE::gWhite->CreateFromData(whiteData, VK_FORMAT_R8G8B8A8_SRGB, ImageSize(1, 1), "Constant white Image");
		CONSTANTS::IMAGE::gBlack = new Image();
		const unsigned char blackData[] = {0, 0, 0, 255};
		CONSTANTS::IMAGE::gBlack->CreateFromData(blackData, VK_FORMAT_R8G8B8A8_SRGB, ImageSize(1, 1), "Constant black Image");
		CONSTANTS::IMAGE::gChecker = new Image();
		// clang-format off
		const unsigned char checkerData[]  = {255, 255, 255, 255, 0, 0, 0, 255, 
									 		 0, 0, 0, 255, 255, 255, 255, 255};
		// clang-format on
		CONSTANTS::IMAGE::gChecker->CreateFromData(checkerData, VK_FORMAT_R8G8B8A8_SRGB, ImageSize(2, 2), "Constant checker Image");
	}

	// imgui
#if defined(ENABLE_IMGUI)
	//sets up gVrGraphics as this, will delete in shutdown
	ImGuiGraphics* imGuiGraphics = new ImGuiGraphics();
	imGuiGraphics->Create(mSurfaces[0]->GetWindow(), mRenderPass);
#endif

#if TRACY_ENABLE
	//buffer needs to stick around?
	//auto buffer = AllocateGraphicsCommandBuffer(false);
	//TracyVkContext(GetVkPhysicalDevice(), GetVkDevice(), GetMainDevice()->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, buffer.mBuffer);
	//EndGraphicsCommandBuffer(buffer, false);
#endif

	LOGGER::Formated("Starting Display with ({}, {}) resolution\n", GetDesiredSize().mWidth, GetDesiredSize().mHeight);
	return true;
}

bool VulkanGraphics::Destroy() {
	ZoneScoped;
	if(gDebugMessenger) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(gVkInstance, "vkDestroyDebugUtilsMessengerEXT");
		ASSERT(func != nullptr);
		if(func != nullptr) {
			func(gVkInstance, gDebugMessenger, GetAllocationCallback());
		}
	}

	vkDestroySampler(GetVkDevice(), mSampler, GetAllocationCallback());
	if(mMaterialManager) {
		mMaterialManager->Destroy();
		delete mMaterialManager;
		mMaterialManager = nullptr;
	}
	// vkFreeDescriptorSets(GetVkDevice(), gDescriptorPool, 1, &gImGuiFontSet);
	vkDestroyDescriptorPool(GetVkDevice(), mDescriptorPool, GetAllocationCallback());
	mSampler = VK_NULL_HANDLE;
	mDescriptorPool = VK_NULL_HANDLE;

#if defined(ENABLE_IMGUI)
	ImGuiGraphics* tempImGuiGraphics = gImGuiGraphics;
	gImGuiGraphics->Destroy();
	delete tempImGuiGraphics;
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

	{
		CONSTANTS::IMAGE::gWhite->Destroy();
		delete CONSTANTS::IMAGE::gWhite;
		CONSTANTS::IMAGE::gWhite = nullptr;
		CONSTANTS::IMAGE::gBlack->Destroy();
		delete CONSTANTS::IMAGE::gBlack;
		CONSTANTS::IMAGE::gBlack = nullptr;
		CONSTANTS::IMAGE::gChecker->Destroy();
		delete CONSTANTS::IMAGE::gChecker;
		CONSTANTS::IMAGE::gChecker = nullptr;
	}

	vmaDestroyAllocator(mAllocator);
	mAllocator = VK_NULL_HANDLE;

	mSwapchain->Destroy();

#if defined(ENABLE_XR)
	//Destory clears gVrGraphics
	VRGraphics* tempVrGraphics = gVrGraphics;
	gVrGraphics->Destroy();
	delete tempVrGraphics;
	LOGGER::Log("~~~~~~~~~~~~~ XR Vulkan doesnt delete a command pool?\n");
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
	FrameMark;
	ZoneScoped;

	mFrameCounter++;

#if defined(ENABLE_IMGUI)
	gImGuiGraphics->StartNewFrame();

	mMaterialManager->ImGuiDraw();
#endif
}
AQUIRES_LOCK(mCommandPoolMutex) void VulkanGraphics::StartGraphicsFrame() {
	ZoneScoped;

	uint32_t index = mSwapchain->GetNextImage();
	{
		ZoneScopedN("Locking Command pool lock");
		mCommandPoolMutex.lock();
	}
	VkCommandBuffer graphics = mDevicesHandler->GetGraphicsCB(GetCurrentImageIndex());
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(graphics, &info);

	if(mMaterialManager) {
		mMaterialManager->NewFrame();
	}

#if defined(ENABLE_XR)
	gVrGraphics->FrameBegin(graphics);
#endif

	//reset viewport and scissor
	VkViewport viewport = {};
	viewport.width = gGraphics->GetMainSwapchain()->GetSize().width;
	viewport.height = gGraphics->GetMainSwapchain()->GetSize().height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(graphics, 0, 1, &viewport);

	VkRect2D scissorRect;
	scissorRect.offset.x = 0;
	scissorRect.offset.y = 0;
	scissorRect.extent.width = viewport.width;
	scissorRect.extent.height = viewport.height;
	vkCmdSetScissor(graphics, 0, 1, &scissorRect);

	mSwapchain->GetImage(GetCurrentImageIndex())
		.SetImageLayout(graphics,
						VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

RELEASES_LOCK(mCommandPoolMutex) void VulkanGraphics::EndFrame() {
	ZoneScoped;
	VkCommandBuffer graphics = mDevicesHandler->GetGraphicsCB(mSwapchain->GetImageIndex());

#if defined(ENABLE_IMGUI)
	gImGuiGraphics->RenderImGui(graphics, mRenderPass, mFramebuffer[GetCurrentImageIndex()]);
#endif

	mSwapchain->GetImage(GetCurrentImageIndex())
		.SetImageLayout(graphics,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
#if defined(ENABLE_XR)
	//gVrGraphics->GetCurrentImage(0)
	//	.SetImageLayout(graphics,
	//					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	//					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	//					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	//					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	//gVrGraphics->GetCurrentImage(1)
	//	.SetImageLayout(graphics,
	//					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	//					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	//					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	//					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
#endif
	vkEndCommandBuffer(graphics);

	ExecuteOneTimeCommandBuffers();

	mSwapchain->SubmitQueue(mDevicesHandler->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, {graphics});

	{
		ZoneScopedN("Unlocking Command pool lock");
		mCommandPoolMutex.unlock();
	}
#if defined(ENABLE_XR)
	gVrGraphics->FrameEnd();
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
	return mXrFramebuffer[aEye][gVrGraphics->GetCurrentImageIndex(aEye)];
}
#endif

AQUIRES_LOCK(mCommandPoolMutex) OneTimeCommandBuffer VulkanGraphics::AllocateGraphicsCommandBuffer(bool aBegin /*= true*/) {
	ZoneScopedN("Allocating graphics CB");
	mCommandPoolMutex.lock();

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = mDevicesHandler->GetGraphicsPool();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer buffer;
	vkAllocateCommandBuffers(GetVkDevice(), &allocateInfo, &buffer);

	if(aBegin) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(buffer, &beginInfo);
	}

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence;
	vkCreateFence(GetVkDevice(), &fenceInfo, GetAllocationCallback(), &fence);

	SetVkName(VK_OBJECT_TYPE_COMMAND_BUFFER, buffer, "Graphics CommandBuffer");
	SetVkName(VK_OBJECT_TYPE_FENCE, fence, "Graphics CommandBuffer Fence");

	OneTimeCommandBuffer otcb;
	otcb.mBuffer = buffer;
	//otcb.mFence = fence;
	return otcb;
}

RELEASES_LOCK(mCommandPoolMutex) void VulkanGraphics::EndGraphicsCommandBuffer(OneTimeCommandBuffer aBuffer, bool aEnd /*= true*/) {
	ZoneScopedN("Ending graphics CB");
	if(aEnd) {
		vkEndCommandBuffer(aBuffer);

		//VkSubmitInfo submitInfo		  = {};
		//submitInfo.sType			  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pCommandBuffers	  = &aBuffer.mBuffer;
		//submitInfo.commandBufferCount = 1;
		//
		//vkQueueSubmit(mDevicesHandler->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, 1, &submitInfo, aBuffer.mFence);
		//
		//vkWaitForFences(GetVkDevice(), 1, &aBuffer.mFence, true, UINT64_MAX);
		//std::unique_lock<std::mutex> lock(mBuffersToSubmitMutex);
		std::unique_lock<LockableBase(std::mutex)> lock(mBuffersToSubmitMutex);
		mBuffersToSubmit.push_back(aBuffer);
		lock.unlock();
	}

	//vkDestroyFence(GetVkDevice(), aBuffer.mFence, GetAllocationCallback());
	//vkFreeCommandBuffers(GetVkDevice(), mDevicesHandler->GetGraphicsPool(), 1, &aBuffer.mBuffer);
	mCommandPoolMutex.unlock();
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
	return gVrGraphics->GetSwapchainFormat();
}
#endif

const ImageSize VulkanGraphics::GetDesiredSize() const {
#if defined(ENABLE_XR)
	return gVrGraphics->GetDesiredSize();
#else
	return GetMainSwapchain()->GetSize();
#endif
}

bool VulkanGraphics::CreateInstance() {
	VkResult result;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Graphics Playground";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = nullptr;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VULKAN_VERSION;

	VkInstanceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<std::string> extensionsTemp;
	std::vector<const char*> extensions;
	std::vector<const char*> layers;

	// glfw
	{
		uint32_t extensionCount = 0;
		const char** glfwExtentensions = Window::GetGLFWVulkanExtentensions(&extensionCount);
		extensions.assign(glfwExtentensions, glfwExtentensions + extensionCount);
	}

	//not needed?
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#if defined(ENABLE_XR)
	std::vector<std::string> xrExtensions = gVrGraphics->GetVulkanInstanceExtensions();
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
			debugCreateInfo.pNext = nullptr;

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

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	createInfo.ppEnabledLayerNames = layers.data();

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
	for(auto layerExtnsions: mLayerExtensions) {
		for(auto extensions: layerExtnsions) {
			if(strcmp(aExtension, extensions.extensionName) == 0) {
				return true;
			}
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

void VulkanGraphics::ExecuteOneTimeCommandBuffers() {
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence;
	vkCreateFence(GetVkDevice(), &fenceInfo, GetAllocationCallback(), &fence);

	std::unique_lock<LockableBase(std::mutex)> lock(mBuffersToSubmitMutex);
	const size_t numBuffers = mBuffersToSubmit.size();
	std::vector<VkSubmitInfo> submitInfos(numBuffers);

	for(int i = 0; i < numBuffers; i++) {
		VkSubmitInfo& submitInfo = submitInfos[i] = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pCommandBuffers = &mBuffersToSubmit[i].mBuffer;
		submitInfo.commandBufferCount = 1;
	}

	if(numBuffers != 0) {
		vkQueueSubmit(mDevicesHandler->GetPrimaryDeviceData().mQueue.mGraphicsQueue.mQueue, numBuffers, submitInfos.data(), fence);
		ZoneScopedN("Waiting for fences");
		vkWaitForFences(GetVkDevice(), 1, &fence, true, UINT64_MAX);
	}

	vkDestroyFence(GetVkDevice(), fence, GetAllocationCallback());
	for(int i = 0; i < numBuffers; i++) {
		vkFreeCommandBuffers(GetVkDevice(), mDevicesHandler->GetGraphicsPool(), 1, &mBuffersToSubmit[i].mBuffer);

		if(mBuffersToSubmit[i].mDataBuffer.GetBuffer() != VK_NULL_HANDLE) {
			//Buffer* buffer = (Buffer*)mBuffersToSubmit[i].mDataBuffer;
			mBuffersToSubmit[i].mDataBuffer.Destroy();
		}
	}
	mBuffersToSubmit.clear();
	lock.unlock();
}

#pragma warning(pop) // 26812