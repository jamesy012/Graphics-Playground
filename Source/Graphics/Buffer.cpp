#include "Buffer.h"

#include "Graphics.h"

void Buffer::Destroy() {
	if (mAllocation) {
		vmaDestroyBuffer(gGraphics->GetAllocator(), mBuffer, mAllocation);
		mAllocation = VK_NULL_HANDLE;
		mBuffer = VK_NULL_HANDLE;
	}
}

void Buffer::CreateFromData(const VkDeviceSize aSize, void* aData) {

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = aSize;

	//customize...
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo allocationInfo = {};

	//customize...
	//allocationInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	allocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT /*| VMA_ALLOCATION_CREATE_MAPPED_BIT*/;
	allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

	vmaCreateBuffer(gGraphics->GetAllocator(), &bufferInfo, &allocationInfo, &mBuffer, &mAllocation, &mAllocationInfo);

	void* data;
	vmaMapMemory(gGraphics->GetAllocator(), mAllocation, &data);

	//comes premapped due to VMA_ALLOCATION_CREATE_MAPPED_BIT
	memcpy(data, aData, mAllocationInfo.size);

	vmaUnmapMemory(gGraphics->GetAllocator(), mAllocation);
}
