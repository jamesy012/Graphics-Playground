#pragma once

#include <vector>

#include "Engine/FileIO.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "RenderPass.h"
#include "Material.h"

class Screenspace {
public:
	void AddMaterialBase(MaterialBase* aMaterial) {
		mPipeline.SetMaterialBase(aMaterial);
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

private:
	Pipeline mPipeline;
	Buffer mIndexBuffer; //todo - constant between screenspace shaders
	RenderPass mRenderPass;
	std::vector<Material> mMaterials;
};