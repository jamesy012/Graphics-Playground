#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class Buffer {
public:
	void Destroy();

	void CreateFromData(const VkDeviceSize aSize, void* aData);

	VkBuffer GetBuffer() const { return mBuffer; }

private:
	VkBuffer mBuffer;

	VmaAllocation mAllocation;
	VmaAllocationInfo mAllocationInfo;
};