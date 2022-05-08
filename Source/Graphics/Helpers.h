#pragma once

#include <vulkan/vulkan.h>

struct ImageSize {
	ImageSize() : mWidth(0), mHeight(0) {};
	ImageSize(const VkExtent2D aSize) : mWidth(aSize.width), mHeight(aSize.height) {};
	ImageSize(uint32_t aWidth, uint32_t aHeight) : mWidth(aWidth), mHeight(aHeight) {};

	operator VkExtent2D() const { return { mWidth, mHeight }; }
	operator VkExtent3D() const { return { mWidth, mHeight, 1 }; }

	uint32_t mWidth;
	uint32_t mHeight;
};