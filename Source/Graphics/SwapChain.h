#pragma once

#include <vulkan/vulkan.h>
#include "Devices.h"

#include <vector>
#include "Image.h"

class SwapChain {
public:
	SwapChain(const DeviceData& aDevice) : mAttachedDevice(aDevice) {};
	void Setup();

private:
	void SetupImages();

	VkSwapchainKHR mSwapChain;
	const DeviceData& mAttachedDevice;
	std::vector<Image> mSwapChainImages;
};