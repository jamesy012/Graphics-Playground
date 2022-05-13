#include "Pipeline.h"

#include "Graphics.h"

bool Pipeline::AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage) {
    //preload shaders??
    FileIO::File file = FileIO::LoadFile(aPath);

    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = aStage;
    stageInfo.pName = "main";

    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.pCode = file.GetAsUInt32();
    moduleInfo.codeSize = file.mSize;

    vkCreateShaderModule(gGraphics->GetVkDevice(), &moduleInfo, GetAllocationCallback(), &stageInfo.module);

    mShaders.push_back(stageInfo);

    return stageInfo.module != VK_NULL_HANDLE;
}  

bool Pipeline::Create() {
    VkPipelineCache pipelineCache;
    {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        vkCreatePipelineCache(gGraphics->GetVkDevice(), &pipelineCacheCreateInfo, GetAllocationCallback(), &pipelineCache);
    }
    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
    {
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    }

    VkGraphicsPipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    createInfo.pStages = mShaders.data();
    createInfo.stageCount = mShaders.size();

    createInfo.pRasterizationState = &rasterizationInfo;

    vkCreateGraphicsPipelines(gGraphics->GetVkDevice(), pipelineCache, 1, &createInfo, GetAllocationCallback(), &mPipeline);

    return false;
}
