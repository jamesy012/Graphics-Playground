#pragma once

#include <vulkan/vulkan.h>

class Image {
public:

	void CreateFromVkImage(VkImage aImage, VkFormat aFormat, VkExtent2D aSize);

	const VkImage GetImage() const { return mImage; }
	const VkImageView GetImageView() const { return mImageView; }
	const VkExtent2D GetImageSize() const { return mSize; }

private:
	void CreateVkImageView(VkFormat aFormat);

	VkImage mImage;
	VkImageView mImageView;
	VkExtent2D mSize;
};