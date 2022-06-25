#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Engine/Transform.h"
#include "Material.h"

class Mesh;

class Model {
public:
	void SetMesh(const Mesh* aMesh) {
		mMesh = aMesh;
	}
    void SetMaterialBase(MaterialBase* aBase);

	void SetNumMaterials(int aCount);
	void SetMaterial(int aIndex, Material aMaterial);

	void Render(VkCommandBuffer aBuffer, VkPipelineLayout aLayout);

	Transform mLocation;

private:
	void UpdateMaterials();

	const Mesh* mMesh;
	struct ModelMaterial {
		Material mMaterial;
		bool mSet = false;
		bool mOverride = false;
	};
	std::vector<ModelMaterial> mMaterials;
    MaterialBase* mMaterialBase;
};