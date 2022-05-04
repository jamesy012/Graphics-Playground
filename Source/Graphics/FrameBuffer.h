#pragma once

#include <vulkan/vulkan.h>

class Image;
class RenderPass;

class Framebuffer {
public:
    void Create(const Image& aImage, const RenderPass& mRenderPassTemplate);

    const VkFramebuffer GetFramebuffer() const { return mFramebuffer; }
    const VkExtent2D GetSize() const { return VkExtent2D(); }

private:
    VkFramebuffer mFramebuffer;

};