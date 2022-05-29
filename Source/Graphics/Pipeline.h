#pragma once

#include <vulkan/vulkan.h>

#include <vector>


#include "Engine/FileIO.h"

class Pipeline {
public:
    bool AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage);

    bool Create(VkRenderPass aPass, const char* aName = 0);
    void Destroy();

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
    VkPipelineCache mPipelineCache;
    VkPipeline mPipeline;

    VkDescriptorSetLayoutBinding mBindings;
    VkDescriptorSetLayoutCreateInfo mLayoutCreateInfo;
    VkDescriptorSetLayout mLayouts;
};