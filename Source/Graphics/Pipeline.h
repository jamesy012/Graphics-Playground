#pragma once

#include <vulkan/vulkan.h>

#include <vector>


#include "Engine/FileIO.h"

class Pipeline {
public:
    bool AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage);

    bool Create(VkRenderPass aPass);

private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaders;

    VkPipeline mPipeline;
};