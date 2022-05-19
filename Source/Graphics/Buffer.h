#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

enum class BufferType {
	IMAGE,
	STAGING,
	VERTEX,
	INDEX
};

class Buffer {
public:
	void Destroy();

	void Create(const BufferType aType, const VkDeviceSize aSize);
	void CreateFromData(const BufferType aType, const VkDeviceSize aSize, void* aData);

	void Resize(const VkDeviceSize aSize, const bool aKeepData);

	VkBuffer GetBuffer() const { return mBuffer; }
	const VkBuffer* GetBufferRef() const { return &mBuffer; }
	const BufferType GetType() const { return mType; }

	void* Map();
	void UnMap();

	void Flush();
	static void Flush(Buffer& aBuffers, uint8_t aCount);

private:
	VkBuffer mBuffer = VK_NULL_HANDLE;
	BufferType mType;

	VmaAllocation mAllocation = VK_NULL_HANDLE;
	VmaAllocationInfo mAllocationInfo = {};

	void* mMappedData = nullptr;
};