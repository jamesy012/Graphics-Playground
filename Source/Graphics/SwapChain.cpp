#include "Swapchain.h"

#include "PlatformDebug.h"
#include "Graphics.h"

extern VkInstance gVkInstance;
extern Graphics* gGraphics;

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};
};

void Swapchain::Setup() {
    const VkSurfaceKHR deviceSurface = mAttachedDevice.mSurfaceUsed;

    SwapchainSupportDetails swapChainSupport;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mAttachedDevice.mPhysicalDevice, deviceSurface,
        &swapChainSupport.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &formatCount,
        nullptr);
    swapChainSupport.formats.resize(formatCount);

    if (formatCount != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(mAttachedDevice.mPhysicalDevice, deviceSurface, &formatCount,
            swapChainSupport.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mAttachedDevice.mPhysicalDevice, deviceSurface,
        &presentModeCount, nullptr);
    swapChainSupport.presentModes.resize(presentModeCount);

    if (presentModeCount != 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            mAttachedDevice.mPhysicalDevice, deviceSurface, &presentModeCount, swapChainSupport.presentModes.data());
    }

    //VkSurfaceFormatKHR surfaceFormat =
    //    ChooseSwapSurfaceFormat(swapChainSupport.formats);
    //VkPresentModeKHR presentMode =
    //    ChooseSwapPresentMode(swapChainSupport.presentModes);
    //VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, mWindow);

    mColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    mSwapchainSize = swapChainSupport.capabilities.currentExtent;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = deviceSurface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = mColorFormat;
    createInfo.imageExtent = mSwapchainSize;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
#if PLATFORM_WINDOWS
    createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
#elif PLATFORM_APPLE
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    {
        std::vector<uint32_t> queueFamilies;
        auto addQueue = [&](int8_t aFamily) {
            for (int i = 0; i < queueFamilies.size(); i++) {
                if (queueFamilies[i] == aFamily) {
                    return;
                }
            }
            queueFamilies.push_back(aFamily);
        };
        
        addQueue(mAttachedDevice.mQueue.mGraphicsQueue.mQueueFamily);
        addQueue(mAttachedDevice.mQueue.mComputeQueue.mQueueFamily);
        addQueue(mAttachedDevice.mQueue.mTransferQueue.mQueueFamily);
        addQueue(mAttachedDevice.mQueue.mPresentQueue.mQueueFamily);

        if (queueFamilies.size() > 1) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = queueFamilies.size();
            createInfo.pQueueFamilyIndices = queueFamilies.data();
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;      // Optional
            createInfo.pQueueFamilyIndices = nullptr;  // Optional
        }
    }

    vkCreateSwapchainKHR(mAttachedDevice.mDevice, &createInfo, nullptr, &mSwapchain);


    SetupImages();
    SetupSyncObjects();
}

void Swapchain::SetupImages() {
    if (mSwapchain == VK_NULL_HANDLE) {
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

    for (int i = 0; i < numImages; i++) {
        mFrameInfo[i].mSwapchainImage.CreateFromVkImage(vulkanImages[i], VK_FORMAT_B8G8R8A8_UNORM, mSwapchainSize);
        mSwapchainImages[i] = &mFrameInfo[i].mSwapchainImage;
    }
}

void Swapchain::SetupSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateSemaphore(mAttachedDevice.mDevice, &semaphoreInfo, nullptr, &mRenderSemaphore);
    vkCreateSemaphore(mAttachedDevice.mDevice, &semaphoreInfo, nullptr, &mPresentSemaphore);
    for (size_t i = 0; i < GetNumBuffers(); i++) {
        PerFrameInfo& data = mFrameInfo[i];
        vkCreateFence(mAttachedDevice.mDevice, &fenceInfo, nullptr, &data.mSubmitFence);
    }
}


const uint32_t Swapchain::GetNextImage() {
    
    VkResult result = vkAcquireNextImageKHR(mAttachedDevice.mDevice, mSwapchain, UINT64_MAX, mPresentSemaphore, nullptr, &mImageIndex);

   //result = vkWaitForFences(mAttachedDevice.mDevice, 1, &mFrameInfo[mImageIndex].mSubmitFence, VK_TRUE, UINT64_MAX);
   //result = vkResetFences(mAttachedDevice.mDevice, 1, &mFrameInfo[mImageIndex].mSubmitFence);

    return mImageIndex;
}

void Swapchain::SubmitQueue(VkQueue aQueue, std::vector<VkCommandBuffer> aCommands) {
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
    vkQueueSubmit(aQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(aQueue);

}

void Swapchain::PresentImage() {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &mRenderSemaphore;

    presentInfo.pImageIndices = &mImageIndex;

    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.swapchainCount = 1;
    
    vkQueuePresentKHR(mAttachedDevice.mQueue.mPresentQueue.mQueue, &presentInfo);


}