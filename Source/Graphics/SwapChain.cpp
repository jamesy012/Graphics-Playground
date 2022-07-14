#include "Swapchain.h"

#include "PlatformDebug.h"
#include "Graphics.h"
#include "Engine/Engine.h"
#include "Engine/Window.h"

#include <glm/common.hpp>

extern VkInstance gVkInstance;
extern Graphics* gGraphics;

//todo implement
const bool gVSYNC = false;

void Swapchain::Setup() {
	const VkSurfaceKHR deviceSurface = mAttachedDevice.mSurfaceUsed;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &mSwapChainSupportDetails.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &formatCount, nullptr);
	mSwapChainSupportDetails.formats.resize(formatCount);

	if(formatCount != 0) {
		vkGetPhysicalDeviceSurfaceFormatsKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &formatCount, mSwapChainSupportDetails.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &presentModeCount, nullptr);
	mSwapChainSupportDetails.presentModes.resize(presentModeCount);

	if(presentModeCount != 0) {
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			mAttachedDevice.mPhysicalDevice, deviceSurface, &presentModeCount, mSwapChainSupportDetails.presentModes.data());
	}

	//https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-2.html?language=en#_Toc445674479:~:text=cpp%2C%20function%20GetSwapChainTransform()-,Selecting%20Presentation%20Mode,-Present%20modes%20determine
	mPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for(int i = 0; i < presentModeCount; i++) {
		if(gVSYNC) {
			if(mSwapChainSupportDetails.presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
				mPresentMode = VK_PRESENT_MODE_FIFO_KHR;
				break;
			}
		}
		if(mSwapChainSupportDetails.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			mPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if(mSwapChainSupportDetails.presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
			mPresentMode = VK_PRESENT_MODE_FIFO_KHR;
			break;
		}
		if(mSwapChainSupportDetails.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			mPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			break;
		}
	}

	//VkSurfaceFormatKHR surfaceFormat =
	//    ChooseSwapSurfaceFormat(swapChainSupport.formats);
	//VkPresentModeKHR presentMode =
	//    ChooseSwapPresentMode(swapChainSupport.presentModes);
	//VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, mWindow);

	mColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

	uint32_t imageCount = mSwapChainSupportDetails.capabilities.minImageCount + 1;
	if(mSwapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > mSwapChainSupportDetails.capabilities.maxImageCount) {
		imageCount = mSwapChainSupportDetails.capabilities.maxImageCount;
	}

	mNumImages = glm::clamp(3u, mSwapChainSupportDetails.capabilities.minImageCount, mSwapChainSupportDetails.capabilities.maxImageCount);

	//mSwapchainSize = swapChainSupport.capabilities.currentExtent;
	ResizeWindow();
}

void Swapchain::Destroy() {

	DestroySyncObjects();

	mFrameInfo.clear();
	mSwapchainImages.clear();

	if(mSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(mAttachedDevice.mDevice, mSwapchain, GetAllocationCallback());
		mSwapchain = VK_NULL_HANDLE;
	}
}

void Swapchain::SetupImages() {
	if(mSwapchain == VK_NULL_HANDLE) {
		ASSERT(false);
		return;
	}

	//this should keep the same number since it was just used to setup swapchain image count
	vkGetSwapchainImagesKHR(mAttachedDevice.mDevice, mSwapchain, &mNumImages, nullptr);

	mSwapchainImages.resize(mNumImages);
	mFrameInfo.resize(mNumImages);

	std::vector<VkImage> vulkanImages(mNumImages);
	vkGetSwapchainImagesKHR(mAttachedDevice.mDevice, mSwapchain, &mNumImages, vulkanImages.data());

	OneTimeCommandBuffer buffer = gGraphics->AllocateGraphicsCommandBuffer();

	for(int i = 0; i < mNumImages; i++) {
		mFrameInfo[i].mSwapchainImage.CreateFromVkImage(vulkanImages[i], VK_FORMAT_B8G8R8A8_UNORM, mSwapchainSize);
		mSwapchainImages[i] = &mFrameInfo[i].mSwapchainImage;

		SetVkName(VK_OBJECT_TYPE_IMAGE, mSwapchainImages[i]->GetImage(), "Swapchain " + std::to_string(i));
		SetVkName(VK_OBJECT_TYPE_IMAGE_VIEW, mSwapchainImages[i]->GetImageView(), "Swapchain View " + std::to_string(i));

		//images start undefined, lets put them in their expected states
		mSwapchainImages[i]->SetImageLayout(
			buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
	}

	gGraphics->EndGraphicsCommandBuffer(buffer);
}

void Swapchain::DestroySyncObjects() {
	for(size_t i = 0; i < GetNumBuffers(); i++) {
		mSwapchainImages[i]->Destroy();
		vkDestroyFence(mAttachedDevice.mDevice, mFrameInfo[i].mSubmitFence, GetAllocationCallback());
	}

	if(mRenderSemaphore) {
		vkDestroySemaphore(mAttachedDevice.mDevice, mRenderSemaphore, GetAllocationCallback());
		mRenderSemaphore = VK_NULL_HANDLE;
	}
	if(mPresentSemaphore) {
		vkDestroySemaphore(mAttachedDevice.mDevice, mPresentSemaphore, GetAllocationCallback());
		mPresentSemaphore = VK_NULL_HANDLE;
	}
}

void Swapchain::SetupSyncObjects() {
	VkSemaphoreCreateInfo semaphoreInfo {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	vkCreateSemaphore(mAttachedDevice.mDevice, &semaphoreInfo, GetAllocationCallback(), &mRenderSemaphore);
	vkCreateSemaphore(mAttachedDevice.mDevice, &semaphoreInfo, GetAllocationCallback(), &mPresentSemaphore);
	SetVkName(VK_OBJECT_TYPE_SEMAPHORE, mRenderSemaphore, "Render Semaphore");
	SetVkName(VK_OBJECT_TYPE_SEMAPHORE, mPresentSemaphore, "Present Semaphore");
	for(size_t i = 0; i < GetNumBuffers(); i++) {
		PerFrameInfo& data = mFrameInfo[i];
		vkCreateFence(mAttachedDevice.mDevice, &fenceInfo, GetAllocationCallback(), &data.mSubmitFence);
		SetVkName(VK_OBJECT_TYPE_FENCE, data.mSubmitFence, "Submit Fence " + std::to_string(i));
	}
}

const uint32_t Swapchain::GetNextImage() {
	ZoneScoped;
	VkResult result = vkAcquireNextImageKHR(mAttachedDevice.mDevice, mSwapchain, UINT64_MAX, mPresentSemaphore, nullptr, &mImageIndex);

	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		ResizeWindow();
		return GetNextImage();
	} else {
		VALIDATEVK(result);
	}

	//result = vkWaitForFences(mAttachedDevice.mDevice, 1, &mFrameInfo[mImageIndex].mSubmitFence, VK_TRUE, UINT64_MAX);
	//result = vkResetFences(mAttachedDevice.mDevice, 1, &mFrameInfo[mImageIndex].mSubmitFence);

	return mImageIndex;
}

void Swapchain::SubmitQueue(VkQueue aQueue, std::vector<VkCommandBuffer> aCommands) {
	ZoneScoped;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pSignalSemaphores = &mRenderSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &mPresentSemaphore;
	submitInfo.waitSemaphoreCount = 1;

	submitInfo.pCommandBuffers = aCommands.data();
	submitInfo.commandBufferCount = aCommands.size();

	const VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &flags;
	//vkQueueSubmit(aQueue, 1, &submitInfo, mFrameInfo[mImageIndex].mSubmitFence);
	VALIDATEVK(vkQueueSubmit(aQueue, 1, &submitInfo, VK_NULL_HANDLE));

	VALIDATEVK(vkQueueWaitIdle(aQueue));
}

void Swapchain::PresentImage() {
	ZoneScoped;
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mRenderSemaphore;

	presentInfo.pImageIndices = &mImageIndex;

	presentInfo.pSwapchains = &mSwapchain;
	presentInfo.swapchainCount = 1;

	VkResult result = vkQueuePresentKHR(mAttachedDevice.mQueue.mPresentQueue.mQueue, &presentInfo);
	if(!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
		if(result == VK_ERROR_OUT_OF_DATE_KHR) {
			ResizeWindow();
		} else {
			VALIDATEVK(result);
		}
	}
}

void Swapchain::ResizeWindow() {
	//
	{
		//mSwapchainSize = aRequestedSize;
		int width, height;
		gEngine->GetWindow()->GetFramebufferSize(&width, &height);
		mSwapchainSize.width = width;
		mSwapchainSize.height = height;
	}
	//ASSERT(mSwapchainSize.width <= mSwapChainSupportDetails.capabilities.maxImageExtent.width);
	//ASSERT(mSwapchainSize.height <= mSwapChainSupportDetails.capabilities.maxImageExtent.height);

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mAttachedDevice.mSurfaceUsed;

	createInfo.minImageCount = mNumImages;
	createInfo.imageFormat = mColorFormat;
	createInfo.imageExtent = mSwapchainSize;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	createInfo.preTransform = mSwapChainSupportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = mPresentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = mSwapchain;

	{
		std::vector<uint32_t> queueFamilies;
		auto addQueue = [&](int8_t aFamily) {
			for(int i = 0; i < queueFamilies.size(); i++) {
				if(queueFamilies[i] == aFamily) {
					return;
				}
			}
			queueFamilies.push_back(aFamily);
		};

		//addQueue(mAttachedDevice.mQueue.mGraphicsQueue.mQueueFamily);
		//addQueue(mAttachedDevice.mQueue.mComputeQueue.mQueueFamily);
		//addQueue(mAttachedDevice.mQueue.mTransferQueue.mQueueFamily);
		addQueue(mAttachedDevice.mQueue.mPresentQueue.mQueueFamily);

		if(queueFamilies.size() > 1) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = queueFamilies.size();
			createInfo.pQueueFamilyIndices = queueFamilies.data();
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}
	}

	VALIDATEVK(vkCreateSwapchainKHR(mAttachedDevice.mDevice, &createInfo, nullptr, &mSwapchain));

	if(createInfo.oldSwapchain) {
		const int oldSwapImages = mSwapchainImages.size();
		for(int i = 0; i < oldSwapImages; i++) {
			mSwapchainImages[i]->Destroy();
		}
		mSwapchainImages.clear();
		vkDestroySwapchainKHR(mAttachedDevice.mDevice, createInfo.oldSwapchain, GetAllocationCallback());
	}

	SetupImages();
	SetupSyncObjects();

	gGraphics->ResizeEvent();
}