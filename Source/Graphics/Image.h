#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "Helpers.h"

class Buffer;

class Image {
public:
	Image() {}
	~Image() {}

	void CreateVkImage(const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	void CreateFromBuffer(const Buffer& aBuffer, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	void CreateFromVkImage(const VkImage aImage, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);

	void Destroy();

	void ChangeImageLayout(const VkCommandBuffer aBuffer, VkImageLayout aNewLayout, VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask);

	const VkImage GetImage() const { return mImage; }
	const VkImageView GetImageView() const { return mImageView; }
	const ImageSize GetImageSize() const { return mSize; }

private:
	void CreateVkImageView(const VkFormat aFormat, const char* aName = 0);

	VkImage mImage;
	VkImageView mImageView;

	ImageSize mSize;
	VkImageLayout mLayout;

	VmaAllocation mAllocation;
	VmaAllocationInfo mAllocationInfo;
};