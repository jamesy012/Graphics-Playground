#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "Helpers.h"
#include "Engine/FileIO.h"

class Buffer;

class Image {
public:
	Image() {}
	~Image() {}

	void CreateVkImage(const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	void CreateFromBuffer(const Buffer& aBuffer, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	void CreateFromVkImage(const VkImage aImage, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	//always 4bit
	void CreateFromData(const void* aData, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);

	void LoadImage(const FileIO::Path aFilePath, const VkFormat aFormat);

	void Destroy();

	void SetImageLayout(const VkCommandBuffer aBuffer, VkImageLayout aOldLayout, VkImageLayout aNewLayout, VkPipelineStageFlags aSrcStageMask,
						   VkPipelineStageFlags aDstStageMask) const;

	const VkImage GetImage() const {
		return mImage;
	}
	const VkImageView GetImageView() const {
		return mImageView;
	}
	const ImageSize GetImageSize() const {
		return mSize;
	}

private:
	void CreateVkImageView(const VkFormat aFormat, const char* aName = 0);

	VkImage mImage = VK_NULL_HANDLE;
	VkImageView mImageView = VK_NULL_HANDLE;

	ImageSize mSize		  = {};

	VmaAllocation mAllocation = VK_NULL_HANDLE;
	VmaAllocationInfo mAllocationInfo = {};
};