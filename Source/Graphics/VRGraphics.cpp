#include "VRGraphics.h"

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>

#include "PlatformDebug.h"
#include "Helpers.h"

//get vulkan info
#include "Graphics.h"
#include "Devices.h"

XrInstance gXrInstance;
XrSystemId gXrSystemId;
XrSession gXrSession;
std::vector<XrExtensionProperties> mXrInstanceExtensions;

const uint8_t NUM_SPACES = 3;
struct SpaceInfo {
	XrSpace mSpace = XR_NULL_HANDLE;
	XrExtent2Df mBounds;
} gSpaces[NUM_SPACES];

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

const VkPhysicalDevice VRGraphics::GetRequestedDevice() const {
	XrResult result;
	PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR;
	VkInstance vkInstance			  = gGraphics->GetVkInstance();
	VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
	if(getXrInstanceProcAddr(&xrGetVulkanGraphicsDeviceKHR, "xrGetVulkanGraphicsDeviceKHR")) {
		result = xrGetVulkanGraphicsDeviceKHR(gXrInstance, gXrSystemId, vkInstance, &vkPhysicalDevice);
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
	XrResult xrResult;
	uint32_t extensionCount = 0;
	xrResult				= xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);
	mXrInstanceExtensions.resize(extensionCount);

	for(int i = 0; i < extensionCount; i++) {
		mXrInstanceExtensions[i].type = XR_TYPE_EXTENSION_PROPERTIES;
	}

	xrResult = xrEnumerateInstanceExtensionProperties(NULL, extensionCount, &extensionCount, mXrInstanceExtensions.data());

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
	xrCreateInstance(&create, &gXrInstance);
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

void VRGraphics::SessionSetup() {
	XrResult result;
	XrSystemGetInfo systemGet = {};
	systemGet.type			  = XR_TYPE_SYSTEM_GET_INFO;
	systemGet.formFactor	  = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	systemGet.next			  = NULL;

	result = xrGetSystem(gXrInstance, &systemGet, &gXrSystemId);

	ASSERT(result == XR_SUCCESS);

	uint32_t viewCount;
	result = xrEnumerateViewConfigurations(gXrInstance, gXrSystemId, 0, &viewCount, nullptr);
	std::vector<XrViewConfigurationType> viewTypeConfigs(viewCount);
	result = xrEnumerateViewConfigurations(gXrInstance, gXrSystemId, viewCount, &viewCount, viewTypeConfigs.data());
	ASSERT(viewCount == 1);
	const XrViewConfigurationType viewType = viewTypeConfigs[0];

	XrInstanceProperties instanceProperties {XR_TYPE_INSTANCE_PROPERTIES};
	result = xrGetInstanceProperties(gXrInstance, &instanceProperties);
	XrSystemProperties systemProperties {XR_TYPE_SYSTEM_PROPERTIES};
	result = xrGetSystemProperties(gXrInstance, gXrSystemId, &systemProperties);

	uint32_t blendCount = 0;
	result				= xrEnumerateEnvironmentBlendModes(gXrInstance, gXrSystemId, viewType, 0, &blendCount, nullptr);
	std::vector<XrEnvironmentBlendMode> environmentBlendModes(blendCount);
	result = xrEnumerateEnvironmentBlendModes(gXrInstance, gXrSystemId, viewType, blendCount, &blendCount, environmentBlendModes.data());

	XrViewConfigurationProperties viewConfigProps = {XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
	result										  = xrGetViewConfigurationProperties(gXrInstance, gXrSystemId, viewType, &viewConfigProps);

	uint32_t viewConfigCount = 0;
	result					 = xrEnumerateViewConfigurationViews(gXrInstance, gXrSystemId, viewType, 0, &viewConfigCount, nullptr);
	std::vector<XrViewConfigurationView> viewConfigs(viewConfigCount);
	result = xrEnumerateViewConfigurationViews(gXrInstance, gXrSystemId, viewType, viewConfigCount, &viewConfigCount, viewConfigs.data());

	CreateActions();

	PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR;
	XrGraphicsRequirementsVulkanKHR graphicsRequirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
	if(getXrInstanceProcAddr(&xrGetVulkanGraphicsRequirementsKHR, "xrGetVulkanGraphicsRequirementsKHR")) {
		result = xrGetVulkanGraphicsRequirementsKHR(gXrInstance, gXrSystemId, &graphicsRequirements);
	}
	//todo xr - verify graphics requirements
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

	XrSessionCreateInfo info = {XR_TYPE_SESSION_CREATE_INFO};
	info.systemId			 = gXrSystemId;
	info.next				 = &vulkanInfo;
	XrResult result			 = xrCreateSession(gXrInstance, &info, &gXrSession);
	ASSERT(result == XR_SUCCESS);
}

void VRGraphics::CreateSpaces() {
	XrResult result;

	uint32_t spaceCount = 0;
	result				= xrEnumerateReferenceSpaces(gXrSession, 0, &spaceCount, nullptr);
	std::vector<XrReferenceSpaceType> refSpaceTypes(spaceCount);
	result = xrEnumerateReferenceSpaces(gXrSession, spaceCount, &spaceCount, refSpaceTypes.data());

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

		XrExtent2Df spaceBounds;
		result = xrGetReferenceSpaceBoundsRect(gXrSession, type, &gSpaces[i].mBounds);
	}

	//Hands
	//xrCreateActionSpace()
}

void VRGraphics::PrepareSwapchainData() {

	xrEnumerateSwapchainFormats(gXrSession, 0, 0, 0);
}
