#include "SwapChain.h"

#include "PlatformDebug.h"
#include "Graphics.h"

extern VkInstance gVkInstance;
extern Graphics* gGraphics;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats{};
    std::vector<VkPresentModeKHR> presentModes{};
};

void SwapChain::Setup() {
    const VkSurfaceKHR deviceSurface = mAttachedDevice.mSurfaceUsed;

    SwapChainSupportDetails swapChainSupport;

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

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = deviceSurface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    createInfo.imageExtent = swapChainSupport.capabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
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

    vkCreateSwapchainKHR(mAttachedDevice.mDevice, &createInfo, nullptr, &mSwapChain);

    SetupImages();
}

void SwapChain::SetupImages() {
    if (mSwapChain == VK_NULL_HANDLE) {
        ASSERT(false);
        return;
    }

    uint32_t numImages;
    vkGetSwapchainImagesKHR(mAttachedDevice.mDevice, mSwapChain, &numImages, nullptr);
    mSwapChainImages.resize(numImages);
    std::vector<VkImage> vulkanImages(numImages);
    vkGetSwapchainImagesKHR(mAttachedDevice.mDevice, mSwapChain, &numImages, vulkanImages.data());

    for (int i = 0; i < numImages; i++) {
        mSwapChainImages[i].CreateFromVkImage(vulkanImages[i], VK_FORMAT_B8G8R8A8_UNORM);
    }
}
