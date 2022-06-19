#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

class Image;

class VRGraphics {
public:
	struct GLMViewInfo {
		glm::vec3 mPos;
		glm::quat mRot;
		glm::vec4 mFov;
	};

	//must be called in this order, each stage depends on the last

	void Startup();
	const VkPhysicalDevice GetRequestedDevice() const;
	void Initalize();

	void FrameBegin(VkCommandBuffer aBuffer);

	const uint8_t GetCurrentImageIndex(uint8_t aEye) const;
	const Image& GetImage(uint8_t aEye, uint8_t aIndex) const;
	const Image& GetCurrentImage(uint8_t aEye) const {
		GetImage(aEye, GetCurrentImageIndex(aEye));
	};

	void GetHeadPoseData(GLMViewInfo& aInfo) const;
	void GetEyePoseData(uint8_t aEye, GLMViewInfo& aInfo) const;

	void FrameEnd();

	const std::vector<std::string> GetVulkanInstanceExtensions() const;
	const std::vector<std::string> GetVulkanDeviceExtensions() const;

	void Destroy();

	VkFormat GetSwapchainFormat() const {
		return mSwapchainFormat;
	}

private:
	//create - instance/setting up session
	void CreateInstance();
	void CreateDebugUtils();
	bool SessionSetup();
	void CreateActions();

	//startup - session and head/hand pos
	//needs device
	void CreateSession();
	void CreateSpaces();
	void PrepareSwapchainData();

	//
	//~ xr info
	//
	std::vector<XrExtensionProperties> mXrInstanceExtensions;
	std::vector<XrApiLayerProperties> mXrLayerProperties;

	//
	//~ general xr session info
	//
	XrSystemId gXrSystemId;
	XrSession gXrSession;
	bool gXrSessionActive = false;

	XrInstanceProperties gInstanceProperties {XR_TYPE_INSTANCE_PROPERTIES};
	XrSystemProperties gSystemProperties {XR_TYPE_SYSTEM_PROPERTIES};

	VkFormat mSwapchainFormat;
};

extern VRGraphics* gVrGraphics;
