#include "VRGraphics.h"

#include <vector>

#include "PlatformDebug.h"
#include "Helpers.h"

#include "Image.h"

//get vulkan info
#include "Graphics.h"
#include "Devices.h"
#pragma region XR Variables

VRGraphics* gVrGraphics = nullptr;

XrInstance gXrInstance;

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
	XrPosef mLastPose;
	XrView mViews[NUM_VIEWS];
	VRGraphics::View mInfos[NUM_VIEWS];
} gSpaces[NUM_SPACES];

//hands
XrPath handSubactionPath[VRGraphics::Side::COUNT]			  = {};
XrActionSet actionSet										  = XR_NULL_HANDLE;
XrAction poseAction											  = XR_NULL_HANDLE;
XrAction grabAction											  = XR_NULL_HANDLE;
XrAction vibrateAction										  = XR_NULL_HANDLE;
XrAction quitAction											  = XR_NULL_HANDLE;
XrSpace handSpace[VRGraphics::Side::COUNT]					  = {XR_NULL_HANDLE};
XrSpaceLocation handLocation[VRGraphics::Side::COUNT]		  = {{XR_TYPE_SPACE_LOCATION}, {XR_TYPE_SPACE_LOCATION}};
VRGraphics::ControllerInfo gHandInfo[VRGraphics::Side::COUNT] = {};

//todo
#pragma endregion

#pragma region XR Debug

#define VALIDATEXR() ASSERT(XR_UNQUALIFIED_SUCCESS(result));

XrDebugUtilsMessengerEXT gXrDebugMessenger = XR_NULL_HANDLE;

static XRAPI_ATTR XrBool32 XRAPI_CALL XrDebugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
													  XrDebugUtilsMessageTypeFlagsEXT messageTypes,
													  const XrDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
	ZoneScopedC(0xFF0000);
	TracyMessage(callbackData->message, strlen(callbackData->message));

	const char* severityText[4] = {"VERBOSE", "INFO", "WARNING", "ERROR"};
	const char* typeText[4]		= {"GENERAL", "VALIDATION", "PERFORMANCE", "CONFORMANCE"};
	std::string severity		= "";
	std::string type			= "";
	for(int i = 0; i < 4; i++) {
		if(messageSeverity & (1 << i)) {
			severity += std::string(";") + severityText[i];
		}
		if(messageTypes & (1 << i)) {
			type += std::string(";") + typeText[i];
		}
	}
	LOGGER::Formated(
		"XR_DEBUG {}: ({}) ({}/{})\n\t {}\n", callbackData->messageId, callbackData->functionName, severity, type, callbackData->message);

	// if (callbackData->messageId != 0) {
	//	ASSERT("XR Error");
	// }

	return XR_FALSE;
}

XrDebugUtilsMessengerCreateInfoEXT GetMessengerCreateInfo() {
	XrDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
	debugCreateInfo.type = XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	// clang-format off
	debugCreateInfo.messageTypes =
		XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
		XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
		XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
	debugCreateInfo.messageSeverities =
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	// clang-format on
	debugCreateInfo.userCallback = XrDebugCallback;
	debugCreateInfo.next		 = nullptr;
	return debugCreateInfo;
}

#pragma endregion

//helper for getting function ptr from XR
bool getXrInstanceProcAddr(void* aFunc, const char* aName) {
	PFN_xrVoidFunction* func = (PFN_xrVoidFunction*)aFunc;
	xrGetInstanceProcAddr(gXrInstance, aName, func);
	ASSERT(*func != nullptr);
	return *func != nullptr;
};

void VRGraphics::Startup() {
	ZoneScoped;
	ASSERT(gVrGraphics == nullptr);
	gVrGraphics = this;
	LOGGER::Log("Loading OpenXR:\n");
	XrVersion version = XR_CURRENT_API_VERSION;
	LOGGER::Formated("\tSDK: {}.{}.{}\n", XR_VERSION_MAJOR(version), XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));

	CreateInstance();

	//CreateDebugUtils();

	SessionSetup();
}

void VRGraphics::Initalize() {
	ZoneScoped;
	CreateSession();

	CreateSpaces();

	CreateActions();

	PrepareSwapchainData();
}

void VRGraphics::FrameBegin(VkCommandBuffer aBuffer) {
	ZoneScoped;
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
				//todo add enum as formattable enum? (check color example in conversion.h)
				LOGGER::Formated("Session state changed to {}\n", stateText[(int)sessionState->state]);

				if(sessionState->state == XR_SESSION_STATE_READY) {
					XrSessionBeginInfo beginInfo		   = {XR_TYPE_SESSION_BEGIN_INFO};
					beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

					result = xrBeginSession(gXrSession, &beginInfo);
					VALIDATEXR();
					if(result == XR_SUCCESS) {
						LOGGER::Log("Begining Xr Session\n");
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
				LOGGER::Log("XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED\n");
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

	XrActionsSyncInfo syncInfo = {XR_TYPE_ACTIONS_SYNC_INFO};
	const XrActiveActionSet activeActionSet {actionSet, XR_NULL_PATH};
	syncInfo.countActiveActionSets = 1;
	syncInfo.activeActionSets	   = &activeActionSet;
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

		//swapchain.mImages[swapchain.mActiveImage].SetImageLayout(aBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	}

	for(int i = 0; i < NUM_SPACES; i++) {
		SpaceInfo& spaceInfo = gSpaces[i];

		XrViewState viewState {XR_TYPE_VIEW_STATE};
		uint32_t viewCapacityInput = NUM_VIEWS;
		uint32_t viewCountOutput;

		XrViewLocateInfo locateInfo		 = {XR_TYPE_VIEW_LOCATE_INFO};
		locateInfo.space				 = spaceInfo.mSpace;
		locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		locateInfo.displayTime			 = gFrameState.predictedDisplayTime;

		result = xrLocateViews(gXrSession, &locateInfo, &viewState, viewCapacityInput, &viewCountOutput, spaceInfo.mViews);
		VALIDATEXR();

		for(int q = 0; q < NUM_VIEWS; q++) {
			ViewConvert(spaceInfo.mViews[q], spaceInfo.mInfos[q]);
		}
	}

	for(int i = 0; i < Side::COUNT; i++) {

		XrActionStateGetInfo getInfo {XR_TYPE_ACTION_STATE_GET_INFO};
		getInfo.subactionPath = handSubactionPath[i];

		//grip
		getInfo.action = grabAction;
		XrActionStateFloat grabValue {XR_TYPE_ACTION_STATE_FLOAT};
		result = xrGetActionStateFloat(gXrSession, &getInfo, &grabValue);
		VALIDATEXR();

		//pose? hand avaliable?
		getInfo.action = poseAction;
		XrActionStatePose poseState {XR_TYPE_ACTION_STATE_POSE};
		result = xrGetActionStatePose(gXrSession, &getInfo, &poseState);
		VALIDATEXR();
		gHandInfo[i].mActive = poseState.isActive;
		//m_input.handActive[hand] = poseState.isActive;

		//quit?
		//getInfo.action		  = quitAction;
		////getInfo.subactionPath = XR_NULL_PATH;
		////getInfo.next		  = nullptr;
		//XrActionStateBoolean quitValue {XR_TYPE_ACTION_STATE_BOOLEAN};
		//result = xrGetActionStateBoolean(gXrSession, &getInfo, &quitValue);
		//VALIDATEXR();
		//if((quitValue.isActive == XR_TRUE) && (quitValue.changedSinceLastSync == XR_TRUE) && (quitValue.currentState == XR_TRUE)) {
		//	xrRequestExitSession(gXrSession);
		//}

		//vibration test?
		if(grabValue.isActive == XR_TRUE) {
			// Scale the rendered hand by 1.0f (open) to 0.5f (fully squeezed).
			//handScale[i] = 1.0f - 0.5f * grabValue.currentState;
			if(grabValue.currentState > 0.9f) {
				XrHapticVibration vibration {XR_TYPE_HAPTIC_VIBRATION};
				vibration.amplitude = 0.5;
				vibration.duration	= XR_MIN_HAPTIC_DURATION;
				vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

				XrHapticActionInfo hapticActionInfo {XR_TYPE_HAPTIC_ACTION_INFO};
				hapticActionInfo.action		   = vibrateAction;
				hapticActionInfo.subactionPath = handSubactionPath[i];
				result						   = xrApplyHapticFeedback(gXrSession, &hapticActionInfo, (XrHapticBaseHeader*)&vibration);
				VALIDATEXR();
			}
		}

		result = xrLocateSpace(handSpace[i], gSpaces[1].mSpace, gFrameState.predictedDisplayTime, &handLocation[i]);
		VALIDATEXR();
		PoseConvert(handLocation[i].pose, gHandInfo[i].mHandPose);
	}
}

const uint8_t VRGraphics::GetCurrentImageIndex(uint8_t aEye) const {
	return gXrSwapchains[aEye].mActiveImage;
}
const Image& VRGraphics::GetImage(uint8_t aEye, uint8_t aIndex) const {
	return gXrSwapchains[aEye].mImages[aIndex];
}

void VRGraphics::GetHeadPoseData(View& aInfo) const {
	aInfo = gSpaces[1].mInfos[0];
}
void VRGraphics::GetEyePoseData(uint8_t aEye, View& aInfo) const {
	aInfo = gSpaces[1].mInfos[aEye];
}

void VRGraphics::GetHandInfo(Side aSide, ControllerInfo& aInfo) const {
	aInfo = gHandInfo[aSide];
}

void VRGraphics::FrameEnd() {
	ZoneScoped;
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

	XrCompositionLayerProjectionView projectionViews[NUM_VIEWS] = {};
	for(int i = 0; i < NUM_VIEWS; i++) {
		projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		//projectionViews[i].fov.angleLeft			= -0.7f;
		//projectionViews[i].fov.angleRight			= 0.7f;
		//projectionViews[i].fov.angleDown			= -0.7f;
		//projectionViews[i].fov.angleUp				= 0.7f;
		//projectionViews[i].pose.orientation.w		= 1.0f;
		projectionViews[i].fov						= gSpaces[1].mViews->fov;
		projectionViews[i].pose						= gSpaces[1].mViews->pose;
		projectionViews[i].subImage.swapchain		= gXrSwapchains[i].mSwapchain;
		projectionViews[i].subImage.imageArrayIndex = 0;
		projectionViews[i].subImage.imageRect		= {
			  0, 0, (int32_t)gViewConfigs[i].recommendedImageRectWidth, (int32_t)gViewConfigs[i].recommendedImageRectHeight};
	}

	XrCompositionLayerProjection projectionLayer			  = {};
	const XrCompositionLayerBaseHeader* projectionLayerHeader = (XrCompositionLayerBaseHeader*)&projectionLayer;
	projectionLayer.type									  = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	projectionLayer.space									  = gSpaces[1].mSpace;
	projectionLayer.viewCount								  = NUM_VIEWS;
	projectionLayer.views									  = projectionViews;
	projectionLayer.layerFlags								  = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;

	XrFrameEndInfo endInfo		 = {XR_TYPE_FRAME_END_INFO};
	endInfo.displayTime			 = gFrameState.predictedDisplayTime;
	endInfo.environmentBlendMode = gBlendMode;

	endInfo.layers	   = &projectionLayerHeader;
	endInfo.layerCount = 1;

	VulkanValidationMessage(416909302, false);
	VulkanValidationMessage(1303270965, false);

	result = xrEndFrame(gXrSession, &endInfo);

	VulkanValidationMessage(416909302, true);
	VulkanValidationMessage(1303270965, true);
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
	ASSERT(gVrGraphics != nullptr);
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
	if(gXrDebugMessenger) {
		PFN_xrDestroyDebugUtilsMessengerEXT xrDestroyDebugUtilsMessengerEXT;
		if(getXrInstanceProcAddr(&xrDestroyDebugUtilsMessengerEXT, "xrDestroyDebugUtilsMessengerEXT")) {
			XrResult result = xrDestroyDebugUtilsMessengerEXT(gXrDebugMessenger);
			VALIDATEXR();
		}
		gXrDebugMessenger = XR_NULL_HANDLE;
	}
	if(gXrInstance != XR_NULL_HANDLE) {
		xrDestroyInstance(gXrInstance);
		gXrInstance = XR_NULL_HANDLE;
	}
	gVrGraphics = nullptr;
}

void VRGraphics::CreateInstance() {
	XrResult result;
	uint32_t extensionCount = 0;
	result					= xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	VALIDATEXR();
	mXrInstanceExtensions.resize(extensionCount);

	for(int i = 0; i < extensionCount; i++) {
		mXrInstanceExtensions[i].type = XR_TYPE_EXTENSION_PROPERTIES;
	}

	result = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, mXrInstanceExtensions.data());
	VALIDATEXR();

#if _DEBUG
	std::vector<char*> extensionReadable(extensionCount);
	LOGGER::Formated("XR Extensions:\n");
	for(int i = 0; i < extensionCount; i++) {
		extensionReadable[i] = mXrInstanceExtensions[i].extensionName;
		LOGGER::Formated("\tFound extension {}\n", extensionReadable[i]);
	}
#endif

	uint32_t layerCount = 0;
	result				= xrEnumerateApiLayerProperties(0, &layerCount, nullptr);
	VALIDATEXR();
	mXrLayerProperties.resize(layerCount);

	for(int i = 0; i < layerCount; i++) {
		mXrLayerProperties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
	}
	result = xrEnumerateApiLayerProperties(layerCount, &layerCount, mXrLayerProperties.data());
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
	//optionalXrExtension(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME);
	//optionalXrExtension(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
	optionalXrExtension(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

	XrInstanceCreateInfo create				  = {};
	create.type								  = XR_TYPE_INSTANCE_CREATE_INFO;
	create.applicationInfo.apiVersion		  = XR_MAKE_VERSION(1, 0, 0);
	create.applicationInfo.applicationVersion = 0;
	create.applicationInfo.engineVersion	  = 0;
	strcpy(create.applicationInfo.applicationName, "Graphics-Playground");
	strcpy(create.applicationInfo.engineName, "Graphics-Playground");
	create.enabledExtensionCount = extensions.size();
	create.enabledExtensionNames = extensions.data();

	XrDebugUtilsMessengerCreateInfoEXT debugCreateInfo = GetMessengerCreateInfo();
	create.next										   = &debugCreateInfo;

	result = xrCreateInstance(&create, &gXrInstance);
	VALIDATEXR();
}

void VRGraphics::CreateDebugUtils() {
	XrDebugUtilsMessengerCreateInfoEXT debugCreateInfo = GetMessengerCreateInfo();

	PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXT;
	if(getXrInstanceProcAddr(&xrCreateDebugUtilsMessengerEXT, "xrCreateDebugUtilsMessengerEXT")) {
		XrResult result = xrCreateDebugUtilsMessengerEXT(gXrInstance, &debugCreateInfo, &gXrDebugMessenger);
		VALIDATEXR();
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

	result = xrGetInstanceProperties(gXrInstance, &gInstanceProperties);
	VALIDATEXR();
	result = xrGetSystemProperties(gXrInstance, gXrSystemId, &gSystemProperties);
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

	mDesiredSize = {gViewConfigs[0].recommendedImageRectWidth, gViewConfigs[0].recommendedImageRectHeight};

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
	XrResult result;

	result = xrStringToPath(gXrInstance, "/user/hand/left", &handSubactionPath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right", &handSubactionPath[Side::RIGHT]);
	VALIDATEXR();

	XrActionSetCreateInfo actionSetInfo = {XR_TYPE_ACTION_SET_CREATE_INFO};
	strcpy(actionSetInfo.actionSetName, "gameplay");
	strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
	actionSetInfo.priority = 0;
	result				   = xrCreateActionSet(gXrInstance, &actionSetInfo, &actionSet);
	VALIDATEXR();

	XrActionCreateInfo actionInfo  = {XR_TYPE_ACTION_CREATE_INFO};
	actionInfo.countSubactionPaths = Side::COUNT;
	actionInfo.subactionPaths	   = handSubactionPath;

	actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
	strcpy_s(actionInfo.actionName, "hand_pose");
	strcpy_s(actionInfo.localizedActionName, "Hand Pose");
	result = xrCreateAction(actionSet, &actionInfo, &poseAction);
	VALIDATEXR();

	actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
	strcpy_s(actionInfo.actionName, "grab_object");
	strcpy_s(actionInfo.localizedActionName, "Grab Object");
	result = xrCreateAction(actionSet, &actionInfo, &grabAction);
	VALIDATEXR();

	actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
	strcpy_s(actionInfo.actionName, "vibrate_hand");
	strcpy_s(actionInfo.localizedActionName, "Vibrate Hand");
	result = xrCreateAction(actionSet, &actionInfo, &vibrateAction);
	VALIDATEXR();

	actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
	strcpy_s(actionInfo.actionName, "quit_session");
	strcpy_s(actionInfo.localizedActionName, "Quit Session");
	result = xrCreateAction(actionSet, &actionInfo, &quitAction);
	VALIDATEXR();

	XrPath selectPath[Side::COUNT];
	XrPath squeezeValuePath[Side::COUNT];
	XrPath squeezeForcePath[Side::COUNT];
	XrPath squeezeClickPath[Side::COUNT];
	XrPath posePath[Side::COUNT];
	XrPath hapticPath[Side::COUNT];
	XrPath menuClickPath[Side::COUNT];
	XrPath bClickPath[Side::COUNT];
	XrPath triggerValuePath[Side::COUNT];
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/select/click", &selectPath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/select/click", &selectPath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/grip/pose", &posePath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/grip/pose", &posePath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/output/haptic", &hapticPath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/output/haptic", &hapticPath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/menu/click", &menuClickPath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/menu/click", &menuClickPath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/b/click", &bClickPath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/b/click", &bClickPath[Side::RIGHT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/left/input/trigger/value", &triggerValuePath[Side::LEFT]);
	VALIDATEXR();
	result = xrStringToPath(gXrInstance, "/user/hand/right/input/trigger/value", &triggerValuePath[Side::RIGHT]);
	VALIDATEXR();
	// Suggest bindings for KHR Simple.
	{
		XrPath khrSimpleInteractionProfilePath;
		result = xrStringToPath(gXrInstance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath);
		VALIDATEXR();
		std::vector<XrActionSuggestedBinding> bindings {{// Fall back to a click input for the grab action.
														 {grabAction, selectPath[Side::LEFT]},
														 {grabAction, selectPath[Side::RIGHT]},
														 {poseAction, posePath[Side::LEFT]},
														 {poseAction, posePath[Side::RIGHT]},
														 {quitAction, menuClickPath[Side::LEFT]},
														 {quitAction, menuClickPath[Side::RIGHT]},
														 {vibrateAction, hapticPath[Side::LEFT]},
														 {vibrateAction, hapticPath[Side::RIGHT]}}};
		XrInteractionProfileSuggestedBinding suggestedBindings {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile	 = khrSimpleInteractionProfilePath;
		suggestedBindings.suggestedBindings		 = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		result									 = xrSuggestInteractionProfileBindings(gXrInstance, &suggestedBindings);
		VALIDATEXR();
	}
	// Suggest bindings for the Vive Controller.
	{
		XrPath viveControllerInteractionProfilePath;
		result = xrStringToPath(gXrInstance, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath);
		VALIDATEXR();
		std::vector<XrActionSuggestedBinding> bindings {{{grabAction, triggerValuePath[Side::LEFT]},
														 {grabAction, triggerValuePath[Side::RIGHT]},
														 {poseAction, posePath[Side::LEFT]},
														 {poseAction, posePath[Side::RIGHT]},
														 {quitAction, menuClickPath[Side::LEFT]},
														 {quitAction, menuClickPath[Side::RIGHT]},
														 {vibrateAction, hapticPath[Side::LEFT]},
														 {vibrateAction, hapticPath[Side::RIGHT]}}};
		XrInteractionProfileSuggestedBinding suggestedBindings {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile	 = viveControllerInteractionProfilePath;
		suggestedBindings.suggestedBindings		 = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		result									 = xrSuggestInteractionProfileBindings(gXrInstance, &suggestedBindings);
		VALIDATEXR();
	}

	XrActionSpaceCreateInfo actionSpaceInfo {XR_TYPE_ACTION_SPACE_CREATE_INFO};
	actionSpaceInfo.action							= poseAction;
	actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
	actionSpaceInfo.subactionPath					= handSubactionPath[Side::LEFT];

	result = xrCreateActionSpace(gXrSession, &actionSpaceInfo, &handSpace[Side::LEFT]);
	VALIDATEXR();
	actionSpaceInfo.subactionPath = handSubactionPath[Side::RIGHT];
	result						  = xrCreateActionSpace(gXrSession, &actionSpaceInfo, &handSpace[Side::RIGHT]);
	VALIDATEXR();

	XrSessionActionSetsAttachInfo attachInfo {XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
	attachInfo.countActionSets = 1;
	attachInfo.actionSets	   = &actionSet;
	result					   = xrAttachSessionActionSets(gXrSession, &attachInfo);
	VALIDATEXR();

	//xrDestroyActionSet;
	//xrDestroyAction;
}

void VRGraphics::CreateSession() {
	XrGraphicsBindingVulkanKHR vulkanInfo = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
	vulkanInfo.instance					  = gGraphics->GetVkInstance();
	vulkanInfo.physicalDevice			  = gGraphics->GetVkPhysicalDevice();
	vulkanInfo.device					  = gGraphics->GetVkDevice();
	vulkanInfo.queueFamilyIndex			  = gGraphics->GetMainDevice()->GetPrimaryDeviceData().mQueue.mPresentQueue.mQueueFamily;
	vulkanInfo.queueIndex				  = 0;
	//vulkanInfo.queueFamilyIndex			  = 0;
	//vulkanInfo.queueIndex				  = 0;

	LOGGER::Log("Creating Xr Session\n");
	XrSessionCreateInfo info = {XR_TYPE_SESSION_CREATE_INFO};
	info.systemId			 = gXrSystemId;
	info.next				 = &vulkanInfo;
	XrResult result			 = xrCreateSession(gXrInstance, &info, &gXrSession);
	VALIDATEXR();
	LOGGER::Log("Created Xr Session\n");
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

#if _DEBUG
	std::vector<VkFormat> vkFormats(formatCounts);
	for(int i = 0; i < formatCounts; i++) {
		vkFormats[i] = (VkFormat)formats[i];
	}
#endif

	mSwapchainFormat = VK_FORMAT_UNDEFINED;
	//get desired swapchain
	{
		//got from debug list above
		const VkFormat desiredFormats[] = {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_SRGB};
		const int count					= sizeof(desiredFormats) / sizeof(VkFormat);
		for(int i = 0; i < count && mSwapchainFormat == VK_FORMAT_UNDEFINED; i++) {
			for(int q = 0; q < formatCounts; q++) {
				if(formats[q] == desiredFormats[i]) {
					mSwapchainFormat = desiredFormats[i];
					break;
				}
			}
		}
	}

	if(mSwapchainFormat == VK_FORMAT_UNDEFINED) {
		LOGGER::Formated("Failed to find a desired swapchain format?\n");
		mSwapchainFormat = (VkFormat)formats[0];
	}

	for(int i = 0; i < NUM_VIEWS; i++) {
		XrSwapchainCreateInfo createInfo {XR_TYPE_SWAPCHAIN_CREATE_INFO};
		createInfo.width	   = gViewConfigs[i].recommendedImageRectWidth;
		createInfo.height	   = gViewConfigs[i].recommendedImageRectHeight;
		createInfo.format	   = mSwapchainFormat;
		createInfo.arraySize   = 1;
		createInfo.faceCount   = 1;
		createInfo.mipCount	   = 1;
		createInfo.sampleCount = 1;
		createInfo.usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.createFlags = XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT;

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
				vkImages[vkImage].image, mSwapchainFormat, {gViewConfigs[i].recommendedImageRectWidth, gViewConfigs[i].recommendedImageRectHeight});

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
