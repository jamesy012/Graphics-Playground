#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "Helpers.h"

class Image;

class VRGraphics {
public:

	struct Pose {
		glm::vec3 mPos;
		glm::quat mRot;
	};

	struct View : public Pose {
		glm::vec4 mFov;
	};

	void PositionConvert(XrVector3f& aFrom, glm::vec3& aTo) {
		aTo.x = aFrom.x;
		aTo.y = -aFrom.y;
		aTo.z = aFrom.z;
		}

	void PoseConvert(XrPosef& aFrom, Pose& aTo) {
			PositionConvert(aFrom.position, aTo.mPos);

		//Rotation when using euler to change rotation
		//aTo.mRot.x = aFrom.orientation.x;
		//aTo.mRot.y = aFrom.orientation.y;
		//aTo.mRot.z = aFrom.orientation.z;
		//aTo.mRot.w = aFrom.orientation.w;
		//const glm::vec3 euler	   = glm::eulerAngles(aTo.mRot);
		//aTo.mRot   = glm::quat(euler * glm::vec3(-1, 1, -1));
		aTo.mRot.x = aFrom.orientation.x;
		aTo.mRot.y = -aFrom.orientation.y;
		aTo.mRot.z = aFrom.orientation.z;
		aTo.mRot.w = -aFrom.orientation.w;
	}
	void ViewConvert(XrView& aFrom, View& aTo) {
		PoseConvert(aFrom.pose, aTo);

		aTo.mFov.x = (aFrom.fov.angleLeft);
		aTo.mFov.y = (aFrom.fov.angleRight);
		aTo.mFov.z = (aFrom.fov.angleDown);
		aTo.mFov.w = (aFrom.fov.angleUp);
	}

	struct ControllerInfo {
		VRGraphics::Pose mPose;
		glm::vec3 mLinearVelocity;
		glm::vec3 mAngularVelocity;
		float mTrigger;
		bool mActive;
	};

	enum Side {
		LEFT,
		RIGHT,
		COUNT
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

	void GetHeadPoseData(View& aInfo) const;
	void GetEyePoseData(uint8_t aEye, View& aInfo) const;
	void GetHandInfo(Side aSide, ControllerInfo& aInfo) const;

	void FrameEnd();

	const std::vector<std::string> GetVulkanInstanceExtensions() const;
	const std::vector<std::string> GetVulkanDeviceExtensions() const;

	void Destroy();

	VkFormat GetSwapchainFormat() const {
		return mSwapchainFormat;
	}

	const ImageSize GetDesiredSize() const {
		return mDesiredSize;
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
	ImageSize mDesiredSize;
};

extern VRGraphics* gVrGraphics;
