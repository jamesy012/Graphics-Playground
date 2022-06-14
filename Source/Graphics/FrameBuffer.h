#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "Helpers.h"

class Image;
class RenderPass;

class Framebuffer {
public:
	void AddImage(const Image* aImage) {
		mLinkedImages.push_back(aImage);
	};
	void Create(const RenderPass& mRenderPassTemplate, const char* aName = 0);
	void Destroy();

	const VkFramebuffer GetFramebuffer() const {
		return mFramebuffer;
	}
	const VkExtent2D GetSize() const {
		return mSize;
	}

private:
	VkFramebuffer mFramebuffer;
	VkExtent2D mSize;
	std::vector<const Image*> mLinkedImages;
};