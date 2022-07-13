#pragma once

#include <vulkan/vulkan.h>
#include "Devices.h"

#include <vector>
#include "Image.h"
#include "Framebuffer.h"
#include "Helpers.h"

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
	void DestroySyncObjects();
	void SetupSyncObjects();

	//works out a good size for this swapchain
	//and resizes it
	void ResizeWindow();

	VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
	const DeviceData& mAttachedDevice;
	VkFormat mColorFormat = VK_FORMAT_UNDEFINED;

	struct PerFrameInfo {
		Image mSwapchainImage = Image();
		//Framebuffer mSwapChainFB;

		VkFence mSubmitFence = VK_NULL_HANDLE;
	};
	VkSemaphore mRenderSemaphore = VK_NULL_HANDLE;
	VkSemaphore mPresentSemaphore = VK_NULL_HANDLE;

	std::vector<PerFrameInfo> mFrameInfo = {};

	uint32_t mImageIndex = 0;

	//remove?
	std::vector<Image*> mSwapchainImages = {};

	VkExtent2D mSwapchainSize = {0, 0};

	//device swapchain details
	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities {};
		std::vector<VkSurfaceFormatKHR> formats {};
		std::vector<VkPresentModeKHR> presentModes {};
	};

	SwapchainSupportDetails mSwapChainSupportDetails;
	uint32_t mNumImages;
	VkPresentModeKHR mPresentMode;
};