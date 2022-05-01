#pragma once

#include <vulkan/vulkan.h>

class Image {
public:

	void CreateFromVkImage(VkImage aImage, VkFormat aFormat);

private:
	void CreateVkImageView(VkFormat aFormat);

	VkImage mImage;
	VkImageView mImageView;
};