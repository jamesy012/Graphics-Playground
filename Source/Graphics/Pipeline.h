#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "PlatformDebug.h"
#include "Engine/FileIO.h"

class MaterialBase;
class Material;

class Pipeline {
public:
	// Pre Create

	bool AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage);
	void AddMaterialBase(MaterialBase* aBase) {
		mMaterials.push_back(aBase);
	};

	void AddPushConstant(const VkPushConstantRange& aPush) {
		mPushConstants.push_back(aPush);
	}
	//pretends to add a material base and push constant
	void AddBindlessTexture() {
		ASSERT(mBindlessTextures == false);
		mBindlessTextures = true;
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

	std::vector<Material> MakeMaterials(uint8_t aBinding) const;

	//Temp
	VkVertexInputBindingDescription vertexBinding = {};
	std::vector<VkVertexInputAttributeDescription> vertexAttribute = {};

private:
	std::vector<VkPipelineShaderStageCreateInfo> mShaders = {};
	std::vector<VkPushConstantRange> mPushConstants = {};
	std::vector<MaterialBase*> mMaterials = {};

	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
	VkPipeline mPipeline = VK_NULL_HANDLE;

	bool mBindlessTextures = false;
};