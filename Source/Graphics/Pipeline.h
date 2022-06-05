#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Engine/FileIO.h"

class MaterialBase;

class Pipeline {
public:
	bool AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage);
	void SetMaterialBase(MaterialBase* aBase) {
		mMaterialBase = aBase;
	};

	void AddPushConstant(VkPushConstantRange aPush) {
		mPushConstants.push_back(aPush);
	}

	bool Create(VkRenderPass aPass, const char* aName = 0);
	void Destroy();

	void Begin(VkCommandBuffer aBuffer);
	void End(VkCommandBuffer aBuffer);

	inline VkPipeline GetPipeline() {
		return mPipeline;
	}
	inline VkPipelineLayout GetLayout() {
		return mPipelineLayout;
	}

private:
	std::vector<VkPipelineShaderStageCreateInfo> mShaders = {};
	std::vector<VkPushConstantRange> mPushConstants		  = {};
	MaterialBase* mMaterialBase							  = nullptr;

	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	VkPipelineCache mPipelineCache	 = VK_NULL_HANDLE;
	VkPipeline mPipeline			 = VK_NULL_HANDLE;
};