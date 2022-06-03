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
	std::vector<VkPipelineShaderStageCreateInfo> mShaders;
	MaterialBase* mMaterialBase = nullptr;

	VkPipelineLayout mPipelineLayout;
	VkPipelineCache mPipelineCache;
	VkPipeline mPipeline;
};