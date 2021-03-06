#pragma once

#include <vector>

#include "Engine/FileIO.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "RenderPass.h"
#include "Material.h"

//makes one color output
class Screenspace {
public:
	struct PushConstant {
		float mEyeIndex;
	};

	void AddMaterialBase(MaterialBase* aMaterial) {
		mPipeline.AddMaterialBase(aMaterial);
	};

	void Create(const FileIO::Path& aFragmentPath, const char* aName = 0);

	void Render(const VkCommandBuffer aBuffer, const Framebuffer& aFramebuffer) const;

	void Destroy();

	const Pipeline& GetPipeline() const {
		return mPipeline;
	}

	const Material& GetMaterial(uint8_t aIndex) const {
		return mMaterials[aIndex];
	}

	void SetClearColors(std::vector<VkClearValue> aClearColors){
		mRenderPass.SetClearColors(aClearColors);
	}


	VkFormat mAttachmentFormat = VK_FORMAT_UNDEFINED;

private:
	Pipeline mPipeline;
	Buffer mIndexBuffer; //todo - constant between screenspace shaders
	RenderPass mRenderPass;
	std::vector<Material> mMaterials;
};