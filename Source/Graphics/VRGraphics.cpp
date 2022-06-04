#include "VRGraphics.h"

#include <vulkan/vulkan.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>

#include "PlatformDebug.h"
#include "Helpers.h"

XrInstance gXrInstance;
XrSystemId gXrSystemId;
std::vector<XrExtensionProperties> mXrInstanceExtensions;

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

	{
		LOG::Log("Loading OpenXR sdk: ");
		uint32_t version = XR_CURRENT_API_VERSION;
		LOG::Log("Version: %i.%i.%i\n", XR_VERSION_MAJOR(version), XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));

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

		XrSystemGetInfo systemGet = {};
		systemGet.type			  = XR_TYPE_SYSTEM_GET_INFO;
		systemGet.formFactor	  = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		systemGet.next			  = NULL;

		xrResult = xrGetSystem(gXrInstance, &systemGet, &gXrSystemId);
	}
}

std::vector<std::string> VRGraphics::GetVulkanInstanceExtensions() {
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

void VRGraphics::Destroy() {
	if(gXrInstance) {
		xrDestroyInstance(gXrInstance);
		gXrInstance = XR_NULL_HANDLE;
	}
}
