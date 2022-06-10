#include "Buffer.h"

#include "Graphics.h"
#include "PlatformDebug.h"

VkBufferUsageFlags GetUsageFromType(const BufferType aType) {
	switch(aType) {
		case BufferType::IMAGE:
			return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			break;
		case BufferType::STAGING:
			return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			break;
		case BufferType::VERTEX:
			return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case BufferType::INDEX:
			return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case BufferType::UNIFORM:
			return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		default:
			ASSERT(false);
	}
	return 0;
}

void Buffer::Destroy() {
	if(mAllocation && mBuffer) {
		vmaDestroyBuffer(gGraphics->GetAllocator(), mBuffer, mAllocation);
		mAllocation = VK_NULL_HANDLE;
		mBuffer		= VK_NULL_HANDLE;
	}
}

void Buffer::Create(const BufferType aType, const VkDeviceSize aSize, const char* aName /* = 0*/) {
	mType = aType;

	if(aSize == 0) {
		return;
	}

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType			  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size				  = aSize;

	//customize...
	bufferInfo.usage = GetUsageFromType(aType);

	VmaAllocationCreateInfo allocationInfo = {};

	//customize...
	//allocationInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	allocationInfo.flags		 = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT /*| VMA_ALLOCATION_CREATE_MAPPED_BIT*/;
	allocationInfo.usage		 = VMA_MEMORY_USAGE_AUTO;

	vmaCreateBuffer(gGraphics->GetAllocator(), &bufferInfo, &allocationInfo, &mBuffer, &mAllocation, &mAllocationInfo);

	SetVkName(VK_OBJECT_TYPE_BUFFER, mBuffer, aName ? aName : "Unnamed Buffer");
}

void Buffer::CreateFromData(const BufferType aType, const VkDeviceSize aSize, const void* aData, const char* aName /* = 0*/) {
	Create(aType, aSize, aName);

	void* data = Map();

	//comes premapped due to VMA_ALLOCATION_CREATE_MAPPED_BIT?
	memcpy(data, aData, mAllocationInfo.size);

	UnMap();
}

void Buffer::Resize(const VkDeviceSize aSize, const bool aKeepData, const char* aName /* = 0*/) {
	ASSERT(aKeepData == false);

	Destroy();
	Create(mType, aSize, aName);
}

void Buffer::UpdateData(const VkDeviceSize aOffset, const VkDeviceSize aSize, const void* aData) {
	char* data = (char*)Map() + aOffset;

	//comes premapped due to VMA_ALLOCATION_CREATE_MAPPED_BIT?
	memcpy(data, aData, aSize == VK_WHOLE_SIZE ? mAllocationInfo.size : aSize);

	UnMap();
}

void* Buffer::Map() {
	ASSERT(mMappedData == nullptr);
	vmaMapMemory(gGraphics->GetAllocator(), mAllocation, &mMappedData);
	return mMappedData;
}
void Buffer::UnMap() {
	vmaUnmapMemory(gGraphics->GetAllocator(), mAllocation);
	mMappedData = nullptr;
}

void Buffer::Flush() {
	vmaFlushAllocation(gGraphics->GetAllocator(), mAllocation, 0, VK_WHOLE_SIZE);
}

void Buffer::Flush(Buffer& aBuffers, uint8_t aCount) {
	ASSERT(false);
	vmaFlushAllocations(gGraphics->GetAllocator(), 0, 0, 0, 0);
}