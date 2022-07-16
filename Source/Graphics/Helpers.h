#pragma once

#include <string>
#include <algorithm>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct MeshPCTest {
	glm::mat4 mWorld;
	//todo revisit texture indexes
	//remove from needing to be part of vertex stage
	//possibly make it material index, which holds other info about the material
	//which could make more sense being part of the vertex stage
	unsigned int mAlbedoTexture;
	unsigned int mNormalTexture;
	unsigned int mMetallicRoughnessTexture;
	float mAlphaCutoff = 0.5f;//making sure the next vec4 is alligned
	glm::vec4 mColorFactor = glm::vec4(0.75f);
	glm::vec2 mMetallicRoughness = glm::vec2(0.25f);
	bool mNormalBC5 = 0; //bc5 textures are incorrect atm
};

struct DirLight {
	glm::vec4 mDirection;

	glm::vec4 mAmbient;
	glm::vec4 mDiffuse;
	glm::vec4 mSpecular;
};


struct SceneData {
	glm::mat4 mPV[2];
	glm::vec4 mViewPos[2];

	DirLight mDirectionalLight;
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

//https://en.cppreference.com/w/cpp/string/byte/tolower
static std::string str_tolower(std::string s) {
	std::transform(s.begin(),
				   s.end(),
				   s.begin(),
				   [](unsigned char c) {
					   return std::tolower(c);
				   }
	);
	return s;
};
