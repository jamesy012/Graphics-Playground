#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

enum class BufferType
{
	IMAGE,
	STAGING,
	VERTEX,
	INDEX,
	UNIFORM
};

class Buffer {
public:
	void Destroy();

	void Create(const BufferType aType, const VkDeviceSize aSize, const char* aName = 0);
	void CreateFromData(const BufferType aType, const VkDeviceSize aSize, const void* aData, const char* aName = 0);

	void Resize(const VkDeviceSize aSize, const bool aKeepData, const char* aName = 0);

	void UpdateData(const VkDeviceSize aOffset, const VkDeviceSize aSize, const void* aData);

	VkBuffer GetBuffer() const {
		return mBuffer;
	}
	const VkBuffer* GetBufferRef() const {
		return &mBuffer;
	}
	const BufferType GetType() const {
		return mType;
	}

	void* Map();
	void UnMap();

	void Flush();
	static void Flush(Buffer& aBuffers, uint8_t aCount);

private:
	VkBuffer mBuffer = VK_NULL_HANDLE;
	BufferType mType;

	VmaAllocation mAllocation		  = VK_NULL_HANDLE;
	VmaAllocationInfo mAllocationInfo = {};

	void* mMappedData = nullptr;
};