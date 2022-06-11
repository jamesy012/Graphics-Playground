#include "VRGraphics.h"

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>

#include "PlatformDebug.h"
#include "Helpers.h"

#include "Image.h"

//get vulkan info
#include "Graphics.h"
#include "Devices.h"
#pragma region XR Variables

//
//~ xr info
//
std::vector<XrExtensionProperties> mXrInstanceExtensions;

//
//~ general xr session info
//
XrInstance gXrInstance;
XrSystemId gXrSystemId;
XrSession gXrSession;
bool gXrSessionActive = false;

//
//~ view/frame xr session info
//

const int NUM_VIEWS = 2;

std::vector<XrViewConfigurationView> gViewConfigs;
XrEnvironmentBlendMode gBlendMode;
struct Swapchain {
	XrSwapchain mSwapchain = XR_NULL_HANDLE;
	std::vector<Image> mImages;
	uint32_t mActiveImage;
} gXrSwapchains[NUM_VIEWS];

//
//~ pre frame xr info
//

XrFrameState gFrameState = {XR_TYPE_FRAME_STATE};

bool gFrameActive = false;

//spaces used to work out head/hand positions
//head space
const uint8_t NUM_SPACES = 3;
struct SpaceInfo {
	XrSpace mSpace = XR_NULL_HANDLE;
	XrExtent2Df mBounds;
} gSpaces[NUM_SPACES];

//hand space
//todo
#pragma endregion

#pragma region XR Debug

#define VALIDATEXR() ASSERT(result == XR_SUCCESS);

XrDebugUtilsMessengerEXT gXrDebugMessenger = XR_NULL_HANDLE;

static XRAPI_ATTR XrBool32 XRAPI_CALL XrDebugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
													  XrDebugUtilsMessageTypeFlagsEXT messageTypes,
													  const XrDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	BitMask maskSeverity = messageSeverity;
	BitMask maskTypes	 = messageTypes;

	const char* severityText[4] = {"VERBOSE", "INFO", "WARNING", "ERROR"};
	const char* typeText[4]		= {"GENERAL", "VALIDATION", "PERFORMANCE", "CONFORMANCE"};
	LOG::Log("XR_DEBUG %s: (%s) (%s/%s)\n\t %s\n",
			 callbackData->messageId,
			 callbackData->functionName,
			 severityText[messageSeverity],
			 typeText[messageTypes],
			 callbackData->message);

	// if (callbackData->messageId != 0) {
	//	ASSERT("XR Error");
	// }

	return XR_FALSE;
}
#pragma endregion

//helper for getting function ptr from XR
bool getXrInstanceProcAddr(void* aFunc, const char* aName) {
	PFN_xrVoidFunction* func = (PFN_xrVoidFunction*)aFunc;
	xrGetInstanceProcAddr(gXrInstance, aName, func);
	ASSERT(*func != nullptr);
	return *func != nullptr;
};

void VRGraphics::Create() {

	LOG::Log("Loading OpenXR sdk: ");
	uint32_t version = XR_CURRENT_API_VERSION;
	LOG::Log("Version: %i.%i.%i\n", XR_VERSION_MAJOR(version), XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));

	CreateInstance();

	CreateDebugUtils();

	SessionSetup();
}

void VRGraphics::Startup() {
	CreateSession();

	CreateSpaces();

	PrepareSwapchainData();
}

void VRGraphics::FrameBegin() {
	XrResult result = XR_SUCCESS;
	XrEventDataBuffer eventData;
	result = xrPollEvent(gXrInstance, &eventData);
	ASSERT(result == XR_SUCCESS || result == XR_EVENT_UNAVAILABLE);
	while(result == XR_SUCCESS) {
		switch(eventData.type) {
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				const char* stateText[] = {"XR_SESSION_STATE_UNKNOWN",
										   "XR_SESSION_STATE_IDLE",
										   "XR_SESSION_STATE_READY",
										   "XR_SESSION_STATE_SYNCHRONIZED",
										   "XR_SESSION_STATE_VISIBLE",
										   "XR_SESSION_STATE_FOCUSED",
										   "XR_SESSION_STATE_STOPPING",
										   "XR_SESSION_STATE_LOSS_PENDING",
										   "XR_SESSION_STATE_EXITING"};

				XrEventDataSessionStateChanged* sessionState = (XrEventDataSessionStateChanged*)&eventData;
				LOG::Log("Session state changed to %s\n", stateText[(int)sessionState->state]);
				if(sessionState->state == XR_SESSION_STATE_READY) {
					XrSessionBeginInfo beginInfo		   = {XR_TYPE_SESSION_BEGIN_INFO};
					beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

					result = xrBeginSession(gXrSession, &beginInfo);
					VALIDATEXR();
					if(result == XR_SUCCESS) {
						LOG::LogLine("Begining Xr Session");
						gXrSessionActive = true;
					}
				}
				if(sessionState->state == XR_SESSION_STATE_STOPPING) {
					result = xrEndSession(gXrSession);
					VALIDATEXR();
					if(result == XR_SUCCESS) {
						gXrSessionActive = false;
					}
				}
				break;
			}
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
				XrEventDataInteractionProfileChanged* profileChange = (XrEventDataInteractionProfileChanged*)&eventData;
				LOG::LogLine("XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED");
				break;
			}
			default:
				ASSERT(false);
		}
		result = xrPollEvent(gXrInstance, &eventData);
		ASSERT(result == XR_SUCCESS || result == XR_EVENT_UNAVAILABLE);
	}

	if(gXrSessionActive == false) {
		return;
	}

	XrActionsSyncInfo syncInfo	   = {XR_TYPE_ACTIONS_SYNC_INFO};
	syncInfo.countActiveActionSets = 0;
	result						   = xrSyncActions(gXrSession, &syncInfo);
	ASSERT(result == XR_SUCCESS || result == XR_SESSION_LOSS_PENDING || result == XR_SESSION_NOT_FOCUSED);
	if(result != XR_SUCCESS && result != XR_SESSION_LOSS_PENDING && result != XR_SESSION_NOT_FOCUSED) {
		return;
	}

	XrFrameWaitInfo frameWaitInfo {XR_TYPE_FRAME_WAIT_INFO};
	result = xrWaitFrame(gXrSession, &frameWaitInfo, &gFrameState);
	VALIDATEXR();

	XrFrameBeginInfo frameBeginInfo {XR_TYPE_FRAME_BEGIN_INFO};
	result = xrBeginFrame(gXrSession, &frameBeginInfo);
	VALIDATEXR();

	gFrameActive = true;

	XrSpaceLocation location = {XR_TYPE_SPACE_LOCATION};
	result					 = xrLocateSpace(gSpaces[XR_REFERENCE_SPACE_TYPE_VIEW - 1].mSpace,
							 gSpaces[XR_REFERENCE_SPACE_TYPE_STAGE - 1].mSpace,
							 gFrameState.predictedDisplayTime,
							 &location);
	VALIDATEXR();

	for(int i = 0; i < NUM_VIEWS; i++) {
		Swapchain& swapchain = gXrSwapchains[i];

		XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

		result = xrAcquireSwapchainImage(swapchain.mSwapchain, &acquireInfo, &swapchain.mActiveImage);
		VALIDATEXR();

		XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
		waitInfo.timeout				  = XR_INFINITE_DURATION;

		result = xrWaitSwapchainImage(swapchain.mSwapchain, &waitInfo);
		VALIDATEXR();
	}
}

const uint8_t VRGraphics::GetCurrentImageIndex(uint8_t aEye) const {
	return gXrSwapchains[aEye].mActiveImage;
}
const Image& VRGraphics::GetImage(uint8_t aEye, uint8_t aIndex) const {
	return gXrSwapchains[aEye].mImages[aIndex];
}

void VRGraphics::FrameEnd() {
	if(gFrameActive == false) {
		return;
	}
	XrResult result;

	for(int i = 0; i < NUM_VIEWS; i++) {
		Swapchain& swapchain = gXrSwapchains[i];

		XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};

		result = xrReleaseSwapchainImage(swapchain.mSwapchain, &releaseInfo);
		VALIDATEXR();
	}

	XrCompositionLayerProjectionView projectionViews[2] = {};

	projectionViews[0].type						= XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
	projectionViews[0].fov.angleLeft			= -0.7f;
	projectionViews[0].fov.angleRight			= 0.7f;
	projectionViews[0].fov.angleDown			= -0.7f;
	projectionViews[0].fov.angleUp				= 0.7f;
	projectionViews[0].pose.orientation.w		= 1.0f;
	projectionViews[0].subImage.swapchain		= gXrSwapchains[0].mSwapchain;
	projectionViews[0].subImage.imageArrayIndex = 0;
	projectionViews[0].subImage.imageRect		= {
		  0, 0, (int32_t)gViewConfigs[0].recommendedImageRectWidth, (int32_t)gViewConfigs[0].recommendedImageRectHeight};
	projectionViews[1].type						= XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
	projectionViews[1].fov.angleLeft			= -0.7f;
	projectionViews[1].fov.angleRight			= 0.7f;
	projectionViews[1].fov.angleDown			= -0.7f;
	projectionViews[1].fov.angleUp				= 0.7f;
	projectionViews[1].pose.orientation.w		= 1.0f;
	projectionViews[1].subImage.swapchain		= gXrSwapchains[1].mSwapchain;
	projectionViews[1].subImage.imageArrayIndex = 0;
	projectionViews[1].subImage.imageRect		= {
		  0, 0, (int32_t)gViewConfigs[1].recommendedImageRectWidth, (int32_t)gViewConfigs[1].recommendedImageRectHeight};

	XrCompositionLayerProjection projectionLayer			  = {};
	const XrCompositionLayerBaseHeader* projectionLayerHeader = (XrCompositionLayerBaseHeader*)&projectionLayer;
	projectionLayer.type									  = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	projectionLayer.space									  = gSpaces[1].mSpace;
	projectionLayer.viewCount								  = NUM_VIEWS;
	projectionLayer.views									  = projectionViews;

	XrFrameEndInfo endInfo		 = {XR_TYPE_FRAME_END_INFO};
	endInfo.displayTime			 = gFrameState.predictedDisplayTime;
	endInfo.environmentBlendMode = gBlendMode;

	endInfo.layers	   = &projectionLayerHeader;
	endInfo.layerCount = 1;

	result = xrEndFrame(gXrSession, &endInfo);
	VALIDATEXR();
}

const VkPhysicalDevice VRGraphics::GetRequestedDevice() const {
	XrResult result;
	PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR;
	VkInstance vkInstance			  = gGraphics->GetVkInstance();
	VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
	if(getXrInstanceProcAddr(&xrGetVulkanGraphicsDeviceKHR, "xrGetVulkanGraphicsDeviceKHR")) {
		result = xrGetVulkanGraphicsDeviceKHR(gXrInstance, gXrSystemId, vkInstance, &vkPhysicalDevice);
		VALIDATEXR();
	};
	return vkPhysicalDevice;
}

const std::vector<std::string> VRGraphics::GetVulkanInstanceExtensions() const {
	std::vector<std::string> output;

	PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensionsKHR;
	getXrInstanceProcAddr(&xrGetVulkanInstanceExtensionsKHR, "xrGetVulkanInstanceExtensionsKHR");

	uint32_t extensionBufferSize = 0;
	XrResult xrResult;
	xrResult		   = xrGetVulkanInstanceExtensionsKHR(gXrInstance, gXrSystemId, 0, &extensionBufferSize, NULL);
	char* xrExtensions = new char[extensionBufferSize];
	xrResult		   = xrGetVulkanInstanceExtensionsKHR(gXrInstance, gXrSystemId, extensionBufferSize, &extensionBufferSize, xrExtensions);

	int lastPoint = 0;
	for(int i = 0; i < extensionBufferSize; i++) {
		if(xrExtensions[i] == ' ') {
			xrExtensions[i] = '\0';
			output.push_back(&xrExtensions[lastPoint]);
			lastPoint = i + 1;
		}
	}
	output.push_back(&xrExtensions[lastPoint]);
	delete[] xrExtensions;

	return output;
}

const std::vector<std::string> VRGraphics::GetVulkanDeviceExtensions() const {
	std::vector<std::string> output;

	PFN_xrGetVulkanDeviceExtensionsKHR xrGetVulkanDeviceExtensionsKHR;
	getXrInstanceProcAddr(&xrGetVulkanDeviceExtensionsKHR, "xrGetVulkanDeviceExtensionsKHR");

	uint32_t extensionBufferSize = 0;
	XrResult xrResult;
	xrResult		   = xrGetVulkanDeviceExtensionsKHR(gXrInstance, gXrSystemId, 0, &extensionBufferSize, NULL);
	char* xrExtensions = new char[extensionBufferSize];
	xrResult		   = xrGetVulkanDeviceExtensionsKHR(gXrInstance, gXrSystemId, extensionBufferSize, &extensionBufferSize, xrExtensions);

	int lastPoint = 0;
	for(int i = 0; i < extensionBufferSize; i++) {
		if(xrExtensions[i] == ' ') {
			xrExtensions[i] = '\0';
			output.push_back(&xrExtensions[lastPoint]);
			lastPoint = i + 1;
		}
	}
	output.push_back(&xrExtensions[lastPoint]);
	delete[] xrExtensions;

	return output;
}

void VRGraphics::Destroy() {
	for(int i = 0; i < NUM_VIEWS; i++) {
		if(gXrSwapchains[i].mSwapchain != XR_NULL_HANDLE) {
			for(int q = 0; q < gXrSwapchains[i].mImages.size(); q++) {
				gXrSwapchains[i].mImages[q].Destroy();
			}
			gXrSwapchains[i].mImages.clear();
			xrDestroySwapchain(gXrSwapchains[i].mSwapchain);
			gXrSwapchains[i].mSwapchain = XR_NULL_HANDLE;
		}
	}
	for(int i = 0; i < NUM_SPACES; i++) {
		if(gSpaces[i].mSpace != XR_NULL_HANDLE) {
			xrDestroySpace(gSpaces[i].mSpace);
			gSpaces[i].mSpace  = XR_NULL_HANDLE;
			gSpaces[i].mBounds = XrExtent2Df();
		}
	}
	if(gXrSession != XR_NULL_HANDLE) {
		xrDestroySession(gXrSession);
		gXrSession = XR_NULL_HANDLE;
	}
	if(gXrInstance != XR_NULL_HANDLE) {
		xrDestroyInstance(gXrInstance);
		gXrInstance = XR_NULL_HANDLE;
	}
}

void VRGraphics::CreateInstance() {
	XrResult result;
	uint32_t extensionCount = 0;
	result					= xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);
	VALIDATEXR();
	mXrInstanceExtensions.resize(extensionCount);

	for(int i = 0; i < extensionCount; i++) {
		mXrInstanceExtensions[i].type = XR_TYPE_EXTENSION_PROPERTIES;
	}

	result = xrEnumerateInstanceExtensionProperties(NULL, extensionCount, &extensionCount, mXrInstanceExtensions.data());
	VALIDATEXR();

	std::vector<const char*> extensions; // = { XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME };

	auto optionalXrExtension = [&](const char* aExtension) {
		for(int i = 0; i < extensionCount; i++) {
			if(strcmp(aExtension, mXrInstanceExtensions[i].extensionName) == 0) {
				extensions.push_back(aExtension);
				return true;
			}
		}
		return false;
	};

	auto requireXrExtension = [&](const char* aExtension) {
		if(optionalXrExtension(aExtension)) {
			return true;
		}
		ASSERT(false);
		return false;
	};

	// if (optionalXrExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME) == false) {
	//	requireXrExtension(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
	// }
	// requireXrExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
	requireXrExtension(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
	optionalXrExtension(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME);
	optionalXrExtension(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
	optionalXrExtension(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

	XrInstanceCreateInfo create				  = {};
	create.type								  = XR_TYPE_INSTANCE_CREATE_INFO;
	create.applicationInfo.apiVersion		  = XR_CURRENT_API_VERSION;
	create.applicationInfo.applicationVersion = 1;
	create.applicationInfo.applicationName;
	strcpy(create.applicationInfo.applicationName, "Graphics-Playground");
	create.enabledExtensionCount = extensions.size();
	create.enabledExtensionNames = extensions.data();
	result						 = xrCreateInstance(&create, &gXrInstance);
	VALIDATEXR();
}

void VRGraphics::CreateDebugUtils() {
	XrDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
	debugCreateInfo.type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverities =
		// XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
	debugCreateInfo.userCallback = XrDebugCallback;
	debugCreateInfo.next		 = nullptr;

	PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXT;
	if(getXrInstanceProcAddr(&xrCreateDebugUtilsMessengerEXT, "xrCreateDebugUtilsMessengerEXT")) {
		xrCreateDebugUtilsMessengerEXT(gXrInstance, &debugCreateInfo, &gXrDebugMessenger);
	}
}

bool VRGraphics::SessionSetup() {
	XrResult result;
	XrSystemGetInfo systemGet = {};
	systemGet.type			  = XR_TYPE_SYSTEM_GET_INFO;
	systemGet.formFactor	  = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	systemGet.next			  = NULL;

	result = xrGetSystem(gXrInstance, &systemGet, &gXrSystemId);
	VALIDATEXR();

	uint32_t viewCount;
	result = xrEnumerateViewConfigurations(gXrInstance, gXrSystemId, 0, &viewCount, nullptr);
	VALIDATEXR();
	std::vector<XrViewConfigurationType> viewTypeConfigs(viewCount);
	result = xrEnumerateViewConfigurations(gXrInstance, gXrSystemId, viewCount, &viewCount, viewTypeConfigs.data());
	VALIDATEXR();
	ASSERT(viewCount == 1);
	ASSERT(viewTypeConfigs[0] == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);
	const XrViewConfigurationType viewType = viewTypeConfigs[0];
	if(viewType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
		ASSERT(false);
		return false;
	}

	XrInstanceProperties instanceProperties {XR_TYPE_INSTANCE_PROPERTIES};
	result = xrGetInstanceProperties(gXrInstance, &instanceProperties);
	VALIDATEXR();
	XrSystemProperties systemProperties {XR_TYPE_SYSTEM_PROPERTIES};
	result = xrGetSystemProperties(gXrInstance, gXrSystemId, &systemProperties);
	VALIDATEXR();

	uint32_t blendCount = 0;
	result				= xrEnumerateEnvironmentBlendModes(gXrInstance, gXrSystemId, viewType, 0, &blendCount, nullptr);
	VALIDATEXR();
	std::vector<XrEnvironmentBlendMode> environmentBlendModes(blendCount);
	result = xrEnumerateEnvironmentBlendModes(gXrInstance, gXrSystemId, viewType, blendCount, &blendCount, environmentBlendModes.data());
	VALIDATEXR();

	for(int i = 0; i < blendCount; i++) {
		if(environmentBlendModes[i] == XrEnvironmentBlendMode::XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
			gBlendMode = environmentBlendModes[i];
			break;
		}
		gBlendMode = environmentBlendModes[i];
	}

	XrViewConfigurationProperties viewConfigProps = {XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
	result										  = xrGetViewConfigurationProperties(gXrInstance, gXrSystemId, viewType, &viewConfigProps);
	VALIDATEXR();

	uint32_t viewConfigCount = 0;
	result					 = xrEnumerateViewConfigurationViews(gXrInstance, gXrSystemId, viewType, 0, &viewConfigCount, nullptr);
	VALIDATEXR();
	gViewConfigs.resize(viewConfigCount);
	result = xrEnumerateViewConfigurationViews(gXrInstance, gXrSystemId, viewType, viewConfigCount, &viewConfigCount, gViewConfigs.data());
	VALIDATEXR();

	CreateActions();

	PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR;
	XrGraphicsRequirementsVulkanKHR graphicsRequirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
	if(getXrInstanceProcAddr(&xrGetVulkanGraphicsRequirementsKHR, "xrGetVulkanGraphicsRequirementsKHR")) {
		result = xrGetVulkanGraphicsRequirementsKHR(gXrInstance, gXrSystemId, &graphicsRequirements);
		VALIDATEXR();
	}
	//todo xr - verify graphics requirements

	return true;
}

void VRGraphics::CreateActions() {
	//todo
	/*
	XrActionSet actionSet				= XR_NULL_HANDLE;
	XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
	strcpy(actionSetInfo.actionSetName, "gameplay");
	strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
	actionSetInfo.priority = 0;
	xrCreateActionSet(gXrInstance, &actionSetInfo, &actionSet);

	XrAction action				  = XR_NULL_HANDLE;
	XrActionCreateInfo actionInfo = {XR_TYPE_ACTION_CREATE_INFO};
	xrCreateAction(actionSet, &actionInfo, &action)
	*/

	//xrDestroyActionSet;
	//xrDestroyAction;
}

void VRGraphics::CreateSession() {
	XrGraphicsBindingVulkanKHR vulkanInfo = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
	vulkanInfo.instance					  = gGraphics->GetVkInstance();
	vulkanInfo.physicalDevice			  = gGraphics->GetVkPhysicalDevice();
	vulkanInfo.device					  = gGraphics->GetVkDevice();
	vulkanInfo.queueFamilyIndex			  = 0;
	vulkanInfo.queueIndex				  = 0;
	//vulkanInfo.queueFamilyIndex					  = gGraphics->GetMainDevice()->GetPrimaryDeviceData().mQueue.mPresentQueue.mQueueFamily;
	//vulkanInfo.queueIndex						  = gGraphics->GetMainDevice()->GetPrimaryDeviceData().mQueue.mQueueFamilies[0].;

	LOG::LogLine("Creating Xr Session");
	XrSessionCreateInfo info = {XR_TYPE_SESSION_CREATE_INFO};
	info.systemId			 = gXrSystemId;
	info.next				 = &vulkanInfo;
	XrResult result			 = xrCreateSession(gXrInstance, &info, &gXrSession);
	VALIDATEXR();
	LOG::LogLine("Created Xr Session");
}

void VRGraphics::CreateSpaces() {
	XrResult result;

	uint32_t spaceCount = 0;
	result				= xrEnumerateReferenceSpaces(gXrSession, 0, &spaceCount, nullptr);
	VALIDATEXR();
	std::vector<XrReferenceSpaceType> refSpaceTypes(spaceCount);
	result = xrEnumerateReferenceSpaces(gXrSession, spaceCount, &spaceCount, refSpaceTypes.data());
	VALIDATEXR();

	//Head
	ASSERT(spaceCount == NUM_SPACES);
	XrReferenceSpaceCreateInfo refSpaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	XrPosef poseReference					= {};
	poseReference.orientation.w				= 1.0f;
	for(int i = 0; i < NUM_SPACES; i++) {
		const XrReferenceSpaceType type	  = XrReferenceSpaceType(refSpaceTypes[i]);
		refSpaceInfo.referenceSpaceType	  = type;
		refSpaceInfo.poseInReferenceSpace = poseReference;
		result							  = xrCreateReferenceSpace(gXrSession, &refSpaceInfo, &gSpaces[i].mSpace);
		VALIDATEXR();

		XrExtent2Df spaceBounds;
		result = xrGetReferenceSpaceBoundsRect(gXrSession, type, &gSpaces[i].mBounds);
		//VALIDATEXR();
	}

	//Hands
	//xrCreateActionSpace()
}

void VRGraphics::PrepareSwapchainData() {
	XrResult result;
	uint32_t formatCounts = 0;
	result				  = xrEnumerateSwapchainFormats(gXrSession, 0, &formatCounts, nullptr);
	VALIDATEXR();
	std::vector<int64_t> formats(formatCounts);
	result = xrEnumerateSwapchainFormats(gXrSession, formatCounts, &formatCounts, formats.data());
	VALIDATEXR();

	VkFormat selectedFormat = (VkFormat)formats[0];

	for(int i = 0; i < NUM_VIEWS; i++) {
		XrSwapchainCreateInfo createInfo {XR_TYPE_SWAPCHAIN_CREATE_INFO};
		createInfo.width	   = gViewConfigs[i].recommendedImageRectWidth;
		createInfo.height	   = gViewConfigs[i].recommendedImageRectHeight;
		createInfo.format	   = selectedFormat;
		createInfo.arraySize   = 1;
		createInfo.faceCount   = 1;
		createInfo.mipCount	   = 1;
		createInfo.sampleCount = 1;
		createInfo.usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

		result = xrCreateSwapchain(gXrSession, &createInfo, &gXrSwapchains[i].mSwapchain);
		VALIDATEXR();
	}

	OneTimeCommandBuffer buffer = gGraphics->AllocateGraphicsCommandBuffer();

	for(int i = 0; i < NUM_VIEWS; i++) {
		Swapchain& swapchain = gXrSwapchains[i];
		uint32_t imageCount;
		result = xrEnumerateSwapchainImages(swapchain.mSwapchain, 0, &imageCount, nullptr);
		VALIDATEXR();
		std::vector<XrSwapchainImageVulkanKHR> vkImages(imageCount);
		//std::vector<XrSwapchainImageBaseHeader> images(imageCount);
		for(int vkImage = 0; vkImage < imageCount; vkImage++) {
			vkImages[vkImage].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
		}
		result =
			xrEnumerateSwapchainImages(swapchain.mSwapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(&vkImages[0]));
		VALIDATEXR();

		gXrSwapchains[i].mImages.resize(imageCount);

		for(int vkImage = 0; vkImage < imageCount; vkImage++) {
			Image& image = gXrSwapchains[i].mImages[vkImage];

			image.CreateFromVkImage(
				vkImages[vkImage].image, selectedFormat, {gViewConfigs[i].recommendedImageRectWidth, gViewConfigs[i].recommendedImageRectHeight});

			SetVkName(VK_OBJECT_TYPE_IMAGE, image.GetImage(), "XR Swapchain eye " + std::to_string(i) + " " + std::to_string(vkImage));
			SetVkName(VK_OBJECT_TYPE_IMAGE_VIEW, image.GetImageView(), "XR Swapchain View eye " + std::to_string(i) + " " + std::to_string(vkImage));

			//images start undefined, lets put them in their expected states
			image.SetImageLayout(buffer,
								 VK_IMAGE_LAYOUT_UNDEFINED,
								 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
								 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
								 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		}
	}

	gGraphics->EndGraphicsCommandBuffer(buffer);
}
