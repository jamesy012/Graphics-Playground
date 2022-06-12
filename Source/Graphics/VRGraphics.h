#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Image;

class VRGraphics {
public:
	struct GLMViewInfo {
		glm::vec3 mPos;
		glm::quat mRot;
		glm::vec4 mFov;
		};

	//must be called in this order, each stage depends on the last

	void Create();
	const VkPhysicalDevice GetRequestedDevice() const;
	void Startup();

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
};