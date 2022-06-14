#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Engine/FileIO.h"

class MaterialBase;
class Material;

class Pipeline {
public:
	// Pre Create

	bool AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage);
	void SetMaterialBase(MaterialBase* aBase) {
		mMaterialBase = aBase;
	};

	void AddPushConstant(const VkPushConstantRange& aPush) {
		mPushConstants.push_back(aPush);
	}

	//create/Destroy

	bool Create(VkRenderPass aPass, const char* aName = 0);
	void Destroy();

	void Begin(VkCommandBuffer aBuffer) const;
	void End(VkCommandBuffer aBuffer) const;

	//helpers

	inline VkPipeline GetPipeline() const {
		return mPipeline;
	}
	inline VkPipelineLayout GetLayout() const {
		return mPipelineLayout;
	}

	std::vector<Material> MakeMaterials() const ;

	//Temp
	VkVertexInputBindingDescription vertexBinding				   = {};
	std::vector<VkVertexInputAttributeDescription> vertexAttribute = {};

private:
	std::vector<VkPipelineShaderStageCreateInfo> mShaders = {};
	std::vector<VkPushConstantRange> mPushConstants		  = {};
	MaterialBase* mMaterialBase							  = nullptr;

	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	VkPipelineCache mPipelineCache	 = VK_NULL_HANDLE;
	VkPipeline mPipeline			 = VK_NULL_HANDLE;
};