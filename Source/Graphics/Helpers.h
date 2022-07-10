#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

struct MeshPCTest {
	glm::mat4 mWorld;
	//todo revisit texture indexes
	//remove from needing to be part of vertex stage
	//possibly make it material index, which holds other info about the material
	//which could make more sense being part of the vertex stage
	unsigned int mAlbedoTexture;
};
struct MeshUniformTest {
	glm::mat4 mPV[2];
};

struct ImageSize {
	ImageSize() : mWidth(0), mHeight(0) {};
	ImageSize(const VkExtent2D aSize) : mWidth(aSize.width), mHeight(aSize.height) {};
	ImageSize(uint32_t aWidth, uint32_t aHeight) : mWidth(aWidth), mHeight(aHeight) {};
	ImageSize(int aWidth, int aHeight) : mWidth((int)aWidth), mHeight((int)aHeight) {};

	bool operator==(const ImageSize& aOther) const {
		return aOther.mHeight == mHeight && aOther.mWidth == mWidth;
	}
	bool operator==(const VkExtent2D& aOther) const {
		return aOther.height == mHeight && aOther.width == mWidth;
	}

	operator VkExtent2D() const {
		return {mWidth, mHeight};
	}
	operator VkExtent3D() const {
		return {mWidth, mHeight, 1};
	}

	uint32_t mWidth;
	uint32_t mHeight;
};

struct BitMask {
	BitMask(uint32_t aValue) : mValue(aValue) {};

	operator uint32_t() const {
		return mValue;
	}

	//BitMask& operator=(const uint32_t& other) {}

	bool operator[](int i) {
		return (mValue & (1 << i));
	}

	bool IsOnlyOneActive() {
		return mValue && !(mValue & (mValue - 1));
	}

	uint32_t mValue;
};