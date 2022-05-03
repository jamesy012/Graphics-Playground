#pragma once

#include <vulkan/vulkan.h>

class RenderPass {
public:
    void Create();
    void Destroy() {};

private:
    VkRenderPass mRenderPass;
};