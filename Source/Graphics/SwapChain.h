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
	void Destroy();

	const uint32_t GetImageIndex() const {
		return mImageIndex;
	};
	const uint32_t GetNextImage();
	void SubmitQueue(VkQueue aQueue, std::vector<VkCommandBuffer> aCommands);
	void PresentImage();

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

		VkFence mSubmitFence;
	};
	VkSemaphore mRenderSemaphore;
	VkSemaphore mPresentSemaphore;

	uint8_t mNumImages = 0;
	std::vector<PerFrameInfo> mFrameInfo;

	uint32_t mImageIndex = 0;

	//remove?
	std::vector<Image*> mSwapchainImages;

	VkExtent2D mSwapchainSize;

};