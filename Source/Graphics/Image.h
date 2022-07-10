#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "Helpers.h"
#include "Engine/FileIO.h"

//for file loading
#include "Engine/Job.h"

class Buffer;

class Image {
public:
	Image() {}
	~Image() {}

	void SetArrayLayers(uint8_t aNumLayers) {
		mArrayLayers = aNumLayers;
	}

	void CreateVkImage(const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	void CreateFromBuffer(Buffer& aBuffer, const bool aDestroyBuffer, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	void CreateFromVkImage(const VkImage aImage, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);
	//always 4bit
	void CreateFromData(const void* aData, const VkFormat aFormat, const ImageSize aSize, const char* aName = 0);

	void LoadImageSync(const FileIO::Path aFilePath, const VkFormat aFormat);
	void LoadImage(const FileIO::Path aFilePath, const VkFormat aFormat);

	void Destroy();

	void SetImageLayout(const VkCommandBuffer aBuffer, VkImageLayout aOldLayout, VkImageLayout aNewLayout,
						VkPipelineStageFlags aSrcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
						VkPipelineStageFlags aDstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT) const;

	const VkImage GetImage() const {
		return mImage;
	}
	const VkImageView GetImageView() const {
		return mImageView;
	}
	const ImageSize GetImageSize() const {
		return mSize;
	}

	const bool HasLoaded() const {
		if(mLoadedHandle == nullptr) {
			return mImageView != VK_NULL_HANDLE;
		}
		if(Job::IsDone(mLoadedHandle)) {
			return mImageView != VK_NULL_HANDLE;
		}
		return false;
		//if(mLoadedHandle){
		//	return Job::IsDone(mLoadingHandle);
		//}
		//return mLoadedHandle == nullptr;
	}

	const int GetGlobalIndex() const {
		return mGlobalTextureIndex;
	}

private:
	void CreateVkImageView(const VkFormat aFormat, const char* aName = 0);

	Job::Work GetLoadImageWork(const FileIO::Path aFilePath, const VkFormat aFormat);

	VkImage mImage = VK_NULL_HANDLE;
	VkImageView mImageView = VK_NULL_HANDLE;

	int mGlobalTextureIndex = -1;

	ImageSize mSize = {};
	VkFormat mFormat;

	uint8_t mArrayLayers = 1;

	VmaAllocation mAllocation = VK_NULL_HANDLE;
	VmaAllocationInfo mAllocationInfo = {};

	Job::WorkHandle* mLoadedHandle = nullptr;
};

namespace CONSTANT {
	namespace IMAGE {
		extern Image* gWhite;
		extern Image* gBlack;
		extern Image* gChecker;
	}; // namespace IMAGE
}; // namespace CONSTANT