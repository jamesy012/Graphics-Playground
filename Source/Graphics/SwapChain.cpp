#include "Swapchain.h"

#include "PlatformDebug.h"
#include "Graphics.h"

extern VkInstance gVkInstance;
extern Graphics* gGraphics;

//todo implement
const bool gVSYNC = false;

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities {};
	std::vector<VkSurfaceFormatKHR> formats {};
	std::vector<VkPresentModeKHR> presentModes {};
};

void Swapchain::Setup(const ImageSize aRequestedSize) {
	const VkSurfaceKHR deviceSurface = mAttachedDevice.mSurfaceUsed;

	SwapchainSupportDetails swapChainSupport;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &swapChainSupport.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &formatCount, nullptr);
	swapChainSupport.formats.resize(formatCount);

	if(formatCount != 0) {
		vkGetPhysicalDeviceSurfaceFormatsKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &formatCount, swapChainSupport.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &presentModeCount, nullptr);
	swapChainSupport.presentModes.resize(presentModeCount);

	if(presentModeCount != 0) {
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			mAttachedDevice.mPhysicalDevice, deviceSurface, &presentModeCount, swapChainSupport.presentModes.data());
	}

	//https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-2.html?language=en#_Toc445674479:~:text=cpp%2C%20function%20GetSwapChainTransform()-,Selecting%20Presentation%20Mode,-Present%20modes%20determine
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for(int i = 0; i < presentModeCount; i++) {
		if(gVSYNC) {
			if(swapChainSupport.presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
				presentMode = VK_PRESENT_MODE_FIFO_KHR;
				break;
			}
		}
		if(swapChainSupport.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if(swapChainSupport.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			break;
		}
	}

	//VkSurfaceFormatKHR surfaceFormat =
	//    ChooseSwapSurfaceFormat(swapChainSupport.formats);
	//VkPresentModeKHR presentMode =
	//    ChooseSwapPresentMode(swapChainSupport.presentModes);
	//VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, mWindow);

	mColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//mSwapchainSize = swapChainSupport.capabilities.currentExtent;
	mSwapchainSize = aRequestedSize;
	ASSERT(mSwapchainSize.width <= swapChainSupport.capabilities.currentExtent.width);
	ASSERT(mSwapchainSize.height <= swapChainSupport.capabilities.currentExtent.height);

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface					= deviceSurface;

	createInfo.minImageCount	= imageCount;
	createInfo.imageFormat		= mColorFormat;
	createInfo.imageExtent		= mSwapchainSize;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	createInfo.preTransform	  = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

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
			createInfo.imageSharingMode		 = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = queueFamilies.size();
			createInfo.pQueueFamilyIndices	 = queueFamilies.data();
		} else {
			createInfo.imageSharingMode		 = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices	 = nullptr; // Optional
		}
	}

	vkCreateSwapchainKHR(mAttachedDevice.mDevice, &createInfo, nullptr, &mSwapchain);

	SetupImages();
	SetupSyncObjects();
}

void Swapchain::Destroy() {
	for(size_t i = 0; i < GetNumBuffers(); i++) {
		mSwapchainImages[i]->Destroy();
		vkDestroyFence(mAttachedDevice.mDevice, mFrameInfo[i].mSubmitFence, GetAllocationCallback());
	}

	mFrameInfo.clear();
	mSwapchainImages.clear();

	if(mRenderSemaphore) {
		vkDestroySemaphore(mAttachedDevice.mDevice, mRenderSemaphore, GetAllocationCallback());
		mRenderSemaphore = VK_NULL_HANDLE;
	}
	if(mPresentSemaphore) {
		vkDestroySemaphore(mAttachedDevice.mDevice, mPresentSemaphore, GetAllocationCallback());
		mPresentSemaphore = VK_NULL_HANDLE;
	}

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

	uint32_t numImages;
	vkGetSwapchainImagesKHR(mAttachedDevice.mDevice, mSwapchain, &numImages, nullptr);

	mNumImages = numImages;
	mSwapchainImages.resize(numImages);
	mFrameInfo.resize(numImages);

	std::vector<VkImage> vulkanImages(numImages);
	vkGetSwapchainImagesKHR(mAttachedDevice.mDevice, mSwapchain, &numImages, vulkanImages.data());

	OneTimeCommandBuffer buffer = gGraphics->AllocateGraphicsCommandBuffer();

	for(int i = 0; i < numImages; i++) {
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

	//result = vkWaitForFences(mAttachedDevice.mDevice, 1, &mFrameInfo[mImageIndex].mSubmitFence, VK_TRUE, UINT64_MAX);
	//result = vkResetFences(mAttachedDevice.mDevice, 1, &mFrameInfo[mImageIndex].mSubmitFence);

	return mImageIndex;
}

void Swapchain::SubmitQueue(VkQueue aQueue, std::vector<VkCommandBuffer> aCommands) {
	ZoneScoped;
	VkSubmitInfo submitInfo			= {};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pSignalSemaphores	= &mRenderSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pWaitSemaphores		= &mPresentSemaphore;
	submitInfo.waitSemaphoreCount	= 1;

	submitInfo.pCommandBuffers	  = aCommands.data();
	submitInfo.commandBufferCount = aCommands.size();

	const VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask	 = &flags;
	//vkQueueSubmit(aQueue, 1, &submitInfo, mFrameInfo[mImageIndex].mSubmitFence);
	vkQueueSubmit(aQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(aQueue);
}

void Swapchain::PresentImage() {
	ZoneScoped;
	VkPresentInfoKHR presentInfo   = {};
	presentInfo.sType			   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores	   = &mRenderSemaphore;

	presentInfo.pImageIndices = &mImageIndex;

	presentInfo.pSwapchains	   = &mSwapchain;
	presentInfo.swapchainCount = 1;

	vkQueuePresentKHR(mAttachedDevice.mQueue.mPresentQueue.mQueue, &presentInfo);
}