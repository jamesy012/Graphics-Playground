#pragma once

#include <vulkan/vulkan.h>
#include "Helpers.h"
class Image;
class RenderPass;

class Framebuffer {
public:
    void Create(const Image& aImage, const RenderPass& mRenderPassTemplate);

    const VkFramebuffer GetFramebuffer() const { return mFramebuffer; }
    const VkExtent2D GetSize() const { return mSize; }

private:
    VkFramebuffer mFramebuffer;
    VkExtent2D mSize;
};