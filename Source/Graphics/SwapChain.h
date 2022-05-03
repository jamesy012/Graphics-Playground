#pragma once

#include <vulkan/vulkan.h>
#include "Devices.h"

#include <vector>
#include "Image.h"

class SwapChain {
public:
	SwapChain(const DeviceData& aDevice) : mAttachedDevice(aDevice) {};
	void Setup();

	const inline uint8_t GetNumBuffers() const {
		return mNumImages;
	}
	const inline VkFormat GetColorFormat() const {
		return mColorFormat;
	}
private:
	void SetupImages();
	void SetupSyncObjects();

	VkSwapchainKHR mSwapChain;
	const DeviceData& mAttachedDevice;
	VkFormat mColorFormat;

	struct PerFrameInfo{
		Image mSwapChainImage;

		VkFence mInFlight;
		VkSemaphore mPresent;
		VkSemaphore mImageReady;
	};

	uint8_t mNumImages = 0;
	std::vector<PerFrameInfo> mFrameInfo;

	//remove?
	std::vector<Image*> mSwapChainImages;

};