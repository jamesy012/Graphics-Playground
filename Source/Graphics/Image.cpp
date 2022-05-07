#include "Image.h"

#include "PlatformDebug.h"
#include "Graphics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

extern Graphics* gGraphics;

void Image::CreateFromVkImage(VkImage aImage, VkFormat aFormat, VkExtent2D aSize) {
	if (aImage == VK_NULL_HANDLE) {
		ASSERT(false);
		return;
	}

	mImage = aImage;
	mSize = aSize;

	CreateVkImageView(aFormat);
}

void Image::CreateVkImageView(VkFormat aFormat) {

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = mImage;
	createInfo.format = aFormat;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	VkImageSubresourceRange colorRange = {};
	colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorRange.baseArrayLayer = 0;
	colorRange.baseMipLevel = 0;
	colorRange.layerCount = 1;
	colorRange.levelCount = 1;
	createInfo.subresourceRange = colorRange;

	vkCreateImageView(gGraphics->GetVkDevice(), &createInfo, GetAllocationCallback(), &mImageView);
}
