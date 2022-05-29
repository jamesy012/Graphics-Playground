#include "Image.h"

#include "PlatformDebug.h"
#include "Graphics.h"
#include "Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Image::CreateVkImage(const VkFormat aFormat, const ImageSize aSize, const char* aName/* = 0*/) {
	VkImageCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.format = aFormat;
	createInfo.extent = aSize;
	createInfo.arrayLayers = 1;
	createInfo.mipLevels = 1;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//vkCreateImage(gGraphics->GetVkDevice(), &createInfo, GetAllocationCallback(), &mImage);

	VmaAllocationCreateInfo allocationInfo{};
	allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(gGraphics->GetAllocator(), &createInfo, &allocationInfo, &mImage, &mAllocation, &mAllocationInfo);

	mSize = aSize;
	mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	CreateVkImageView(aFormat, aName);

	SetVkName(VK_OBJECT_TYPE_IMAGE, mImage, aName ? aName : "Unnamed Image");

}

void Image::CreateFromBuffer(const Buffer& aBuffer, const VkFormat aFormat, const ImageSize aSize, const char* aName/* = 0*/) {
	ASSERT(aBuffer.GetType() == BufferType::STAGING)
	CreateVkImage(aFormat, aSize, aName);

	OneTimeCommandBuffer cmBuffer = gGraphics->AllocateGraphicsCommandBuffer();

	ChangeImageLayout(cmBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkBufferImageCopy imageCopy = {};
	imageCopy.imageSubresource.layerCount = 1;
	imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopy.imageExtent = mSize;

	vkCmdCopyBufferToImage(cmBuffer, aBuffer.GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

	ChangeImageLayout(cmBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	gGraphics->EndGraphicsCommandBuffer(cmBuffer);
}

void Image::CreateFromVkImage(const VkImage aImage, const VkFormat aFormat, const ImageSize aSize, const char* aName/* = 0*/) {
	if (aImage == VK_NULL_HANDLE) {
		ASSERT(false);
		return;
	}

	mImage = aImage;
	mSize = aSize;
	//pass through?
	mLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	CreateVkImageView(aFormat, aName);
}

void Image::Destroy() {

	if (mImageView) {
		vkDestroyImageView(gGraphics->GetVkDevice(), mImageView, GetAllocationCallback());
		mImageView = VK_NULL_HANDLE;
	}

	if (mAllocation) {
		vmaDestroyImage(gGraphics->GetAllocator(), mImage, mAllocation);
		mAllocation = VK_NULL_HANDLE;
		mImage = VK_NULL_HANDLE;
	}
}

void Image::ChangeImageLayout(const VkCommandBuffer aBuffer, VkImageLayout aNewLayout, VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask) {
	VkImageMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memoryBarrier.oldLayout = mLayout;
	memoryBarrier.newLayout = aNewLayout;
	memoryBarrier.image = mImage;
	memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	memoryBarrier.subresourceRange.baseArrayLayer = 0;
	memoryBarrier.subresourceRange.baseMipLevel = 0;
	memoryBarrier.subresourceRange.layerCount = 1;
	memoryBarrier.subresourceRange.levelCount = 1;

	switch (mLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		memoryBarrier.srcAccessMask = 0;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		ASSERT(false);
	}

	switch (aNewLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_UNDEFINED:
		memoryBarrier.dstAccessMask = VK_ACCESS_NONE;
		break;
	default:
		ASSERT(false);
	}

	vkCmdPipelineBarrier(aBuffer, aSrcStageMask, aDstStageMask, 0, 0, 0, 0, 0, 1, &memoryBarrier);

	mLayout = aNewLayout;
}

void Image::CreateVkImageView(const VkFormat aFormat, const char* aName/* = 0*/) {

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
	SetVkName(VK_OBJECT_TYPE_IMAGE_VIEW, mImageView, aName ? aName : "Unnamed ImageView");
}
