#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan.h>

class VRGraphics {
public:
	//must be called in this order, each stage depends on the last

	void Create();
	const VkPhysicalDevice GetRequestedDevice() const;
	void Startup();


	const std::vector<std::string> GetVulkanInstanceExtensions() const;
	const std::vector<std::string> GetVulkanDeviceExtensions() const;

	void Destroy();
private:
	//create - instance/setting up session
	void CreateInstance();
	void CreateDebugUtils();
	void SessionSetup();
	void CreateActions();

	//startup - session and head/hand pos 
	//needs device
	void CreateSession();
	void CreateSpaces();
	void PrepareSwapchainData();

	VkPhysicalDevice mRequestedDevice;
};