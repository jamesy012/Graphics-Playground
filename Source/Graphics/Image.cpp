#include "Image.h"

#include "PlatformDebug.h"
#include "Graphics.h"
#include "Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace CONSTANT {
	namespace IMAGE {
		Image* gWhite	= nullptr;
		Image* gBlack	= nullptr;
		Image* gChecker = nullptr;
	}; // namespace IMAGE
}; // namespace CONSTANT

uint32_t ConvertImageSizeToByteSize(ImageSize aSize) {
	return aSize.mWidth * aSize.mHeight * sizeof(char) * 4;
}

void Image::CreateVkImage(const VkFormat aFormat, const ImageSize aSize, const char* aName /* = 0*/) {
	VkFormat format;
	if(aFormat == VK_FORMAT_UNDEFINED) {
		format = Graphics::GetDeafultColorFormat();
	} else {
		format = aFormat;
	}

	VkImageCreateInfo createInfo {};
	createInfo.sType		 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.format		 = format;
	createInfo.extent		 = aSize;
	createInfo.arrayLayers	 = mArrayLayers;
	createInfo.mipLevels	 = 1;
	createInfo.sharingMode	 = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples		 = VK_SAMPLE_COUNT_1_BIT;
	createInfo.imageType	 = VK_IMAGE_TYPE_2D;
	createInfo.tiling		 = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage		 = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if(Graphics::IsFormatDepth(format)) {
		createInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	} else {
		createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	//vkCreateImage(gGraphics->GetVkDevice(), &createInfo, GetAllocationCallback(), &mImage);

	VmaAllocationCreateInfo allocationInfo {};
	allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(gGraphics->GetAllocator(), &createInfo, &allocationInfo, &mImage, &mAllocation, &mAllocationInfo);

	mSize	= aSize;
	mFormat = format;

	CreateVkImageView(format, aName);

	SetVkName(VK_OBJECT_TYPE_IMAGE, mImage, aName ? aName : "Unnamed Image");
}

void Image::CreateFromBuffer(Buffer& aBuffer, const bool aDestroyBuffer, const VkFormat aFormat, const ImageSize aSize,
							 const char* aName /* = 0*/) {
	ZoneScoped;
	ASSERT(aBuffer.GetType() == BufferType::STAGING)
	CreateVkImage(aFormat, aSize, aName);

	OneTimeCommandBuffer cmBuffer = gGraphics->AllocateGraphicsCommandBuffer();
	{
		ZoneScopedN("Has Lock");
		SetImageLayout(
			cmBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkBufferImageCopy imageCopy			  = {};
		imageCopy.imageSubresource.layerCount = mArrayLayers;
		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageExtent				  = mSize;

		vkCmdCopyBufferToImage(cmBuffer, aBuffer.GetBuffer(), mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

		SetImageLayout(cmBuffer,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					   VK_PIPELINE_STAGE_TRANSFER_BIT,
					   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		if(aDestroyBuffer) {
			cmBuffer.mDataBuffer = aBuffer;
		}
	}
	gGraphics->EndGraphicsCommandBuffer(cmBuffer);
}

void Image::CreateFromVkImage(const VkImage aImage, const VkFormat aFormat, const ImageSize aSize, const char* aName /* = 0*/) {
	if(aImage == VK_NULL_HANDLE) {
		ASSERT(false);
		return;
	}
	ASSERT(aFormat != VK_FORMAT_UNDEFINED);

	mImage	= aImage;
	mSize	= aSize;
	mFormat = aFormat;

	CreateVkImageView(aFormat, aName);
}

void Image::CreateFromData(const void* aData, const VkFormat aFormat, const ImageSize aSize, const char* aName /* = 0*/) {
	ZoneScoped;
	Buffer dataBuffer;
	dataBuffer.CreateFromData(BufferType::STAGING, ConvertImageSizeToByteSize(aSize), aData, aName);

	CreateFromBuffer(dataBuffer, true, aFormat, aSize, aName);

	//dataBuffer.Destroy();
}

struct AsyncLoadDataImage {
	stbi_uc* mData;
	int width, height, comp;
	Image* ptr;
};

void Image::LoadImageSync(const FileIO::Path aFilePath, const VkFormat aFormat) {
	Job::Work work = GetLoadImageWork(aFilePath, aFormat);
	work.DoWork();
}
void Image::LoadImage(const FileIO::Path aFilePath, const VkFormat aFormat) {
	Job::Work work = GetLoadImageWork(aFilePath, aFormat);
	mLoadedHandle  = Job::QueueWorkHandle(work);
}

Job::Work Image::GetLoadImageWork(const FileIO::Path aFilePath, const VkFormat aFormat) {
	Job::Work work;
	AsyncLoadDataImage* imageData = new AsyncLoadDataImage();
	imageData->ptr			 = this;

	work.mUserData = imageData;
	work.mWorkPtr  = [aFilePath](void* userData) {
		 ZoneScoped;
		 ZoneText(aFilePath.String().c_str(), aFilePath.String().size());
		 AsyncLoadDataImage* imageData = (AsyncLoadDataImage*)userData;
		 //todo
		 imageData->mData = stbi_load(aFilePath.String().c_str(), &imageData->width, &imageData->height, &imageData->comp, STBI_rgb_alpha);
		 ASSERT(imageData->mData != nullptr);
		 //ASSERT(imageData->comp == 4);
		 ZoneValue(imageData->width);
		 ZoneValue(imageData->height);

	};
	//work.mFinishOnMainThread = true;
	work.mFinishPtr = [=](void* userData) {
		ZoneScoped;
		AsyncLoadDataImage* imageData = (AsyncLoadDataImage*)userData;
		imageData->ptr->CreateFromData(imageData->mData, aFormat, {imageData->width, imageData->height}, aFilePath.String().c_str());
		stbi_image_free(imageData->mData);
	};
	return work;
}

void Image::Destroy() {

	if(mImageView) {
		vkDestroyImageView(gGraphics->GetVkDevice(), mImageView, GetAllocationCallback());
		mImageView = VK_NULL_HANDLE;
	}

	if(mAllocation) {
		vmaDestroyImage(gGraphics->GetAllocator(), mImage, mAllocation);
		mAllocation = VK_NULL_HANDLE;
		mImage		= VK_NULL_HANDLE;
	}
}

void Image::SetImageLayout(const VkCommandBuffer aBuffer, VkImageLayout aOldLayout, VkImageLayout aNewLayout, VkPipelineStageFlags aSrcStageMask,
						   VkPipelineStageFlags aDstStageMask) const {
	VkImageMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType				   = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memoryBarrier.oldLayout			   = aOldLayout;
	memoryBarrier.newLayout			   = aNewLayout;
	memoryBarrier.image				   = mImage;
	if(Graphics::IsFormatDepth(mFormat)) {
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else {
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	memoryBarrier.subresourceRange.baseArrayLayer = 0;
	memoryBarrier.subresourceRange.baseMipLevel	  = 0;
	memoryBarrier.subresourceRange.layerCount	  = mArrayLayers;
	memoryBarrier.subresourceRange.levelCount	  = 1;

	switch(aOldLayout) {
		case VK_IMAGE_LAYOUT_UNDEFINED:
			memoryBarrier.srcAccessMask = VK_ACCESS_NONE_KHR; //should this be something else?
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			memoryBarrier.srcAccessMask = VK_ACCESS_NONE_KHR; //should this be something else?
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT; //should this be something else?
			break;
		default:
			ASSERT(false);
	}

	switch(aNewLayout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //should this be something else?
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; //should this be something else?
			break;
		case VK_IMAGE_LAYOUT_UNDEFINED:
			memoryBarrier.dstAccessMask = VK_ACCESS_NONE_KHR;
			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			memoryBarrier.dstAccessMask = VK_ACCESS_NONE_KHR; //should this be something else?
			break;
		default:
			ASSERT(false);
	}

	vkCmdPipelineBarrier(aBuffer, aSrcStageMask, aDstStageMask, 0, 0, 0, 0, 0, 1, &memoryBarrier);
}

void Image::CreateVkImageView(const VkFormat aFormat, const char* aName /* = 0*/) {

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType				 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image				 = mImage;
	createInfo.format				 = aFormat;
	if(mArrayLayers > 1) {
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	} else {
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	}

	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.baseMipLevel   = 0;
	createInfo.subresourceRange.layerCount	   = mArrayLayers;
	createInfo.subresourceRange.levelCount	   = 1;

	if(Graphics::IsFormatDepth(aFormat)) {
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else {
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	VkResult result;
	result = vkCreateImageView(gGraphics->GetVkDevice(), &createInfo, GetAllocationCallback(), &mImageView);
	SetVkName(VK_OBJECT_TYPE_IMAGE_VIEW, mImageView, aName ? aName : "Unnamed ImageView");
}
