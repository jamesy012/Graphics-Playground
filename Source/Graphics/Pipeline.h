#pragma once

#include <vulkan/vulkan.h>

#include <vector>


#include "Engine/FileIO.h"

class Pipeline {
public:
    bool AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage);

    bool Create(VkRenderPass aPass);

    void Begin(VkCommandBuffer aBuffer);
    void End(VkCommandBuffer aBuffer);


    inline VkPipeline GetPipeline(){
        return mPipeline;
    }
    inline VkPipelineLayout GetLayout(){
        return mPipelineLayout;
    }

    VkDescriptorSetLayout GetSetLayout() {
       return mLayouts;
    }
private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaders;

    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;

    VkDescriptorSetLayoutBinding mBindings;
    VkDescriptorSetLayoutCreateInfo mLayoutCreateInfo;
    VkDescriptorSetLayout mLayouts;
};