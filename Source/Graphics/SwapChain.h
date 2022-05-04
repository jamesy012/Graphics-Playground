#pragma once

#include <vulkan/vulkan.h>
#include "Devices.h"

#include <vector>
#include "Image.h"
#include "Framebuffer.h"

class Swapchain {
public:
	Swapchain(const DeviceData& aDevice) : mAttachedDevice(aDevice) {};
	void Setup();

	const int GetNextImage();
	void PresentImage(const int aIndex);

	const inline uint8_t GetNumBuffers() const {
		return mNumImages;
	}
	const inline VkFormat GetColorFormat() const {
		return mColorFormat;
	}

	const Image& GetImage(uint8_t aIndex) const {
		return mFrameInfo[aIndex].mSwapchainImage;		
	}
	const VkExtent2D& GetSize() const {
		return mSwapchainSize;		
	}
private:
	void SetupImages();
	void SetupSyncObjects();

	VkSwapchainKHR mSwapchain;
	const DeviceData& mAttachedDevice;
	VkFormat mColorFormat;

	struct PerFrameInfo{
		Image mSwapchainImage;
		//Framebuffer mSwapChainFB;

		VkFence mInFlight;
		VkSemaphore mPresent;
		VkSemaphore mImageReady;
	};

	uint8_t mNumImages = 0;
	std::vector<PerFrameInfo> mFrameInfo;

	//remove?
	std::vector<Image*> mSwapchainImages;

	VkExtent2D mSwapchainSize;

};