#pragma once

#include <vulkan/vulkan.h>

class Framebuffer;

class RenderPass {
public:
    void Create(const char* aName = 0);
    void Destroy();

    void Begin(VkCommandBuffer aBuffer, const Framebuffer& aFramebuffer);
    void End(VkCommandBuffer aBuffer);

    const VkRenderPass GetRenderPass() const {
        return mRenderPass;
    }
private:
    VkRenderPass mRenderPass;
};