#include "Model.h"

#include "Mesh.h"
#include "PlatformDebug.h"

void Model::SetMaterialBase(MaterialBase* aBase) {
    mMaterialBase = aBase;
}

void Model::SetNumMaterials(int aCount) {
	mMaterials.resize(aCount);
}
void Model::SetMaterial(int aIndex, Material aMaterial) {
	assert(aIndex < mMaterials.size());
	ModelMaterial& material = mMaterials[aIndex];
	material.mMaterial		= aMaterial;
	material.mOverride		= true;
	material.mSet			= true;
}

void Model::Render(VkCommandBuffer aBuffer, VkPipelineLayout aLayout) {
	if(!mMesh->HasLoaded()) {
		return;
	}
	UpdateMaterials();
	const int numMesh = mMesh->GetNumMesh();

	for(int i = 0; i < numMesh; i++) {
		const Mesh::SubMesh& mesh = mMesh->GetMesh(i);
		if(mesh.mMaterialID != -1) {
			const ModelMaterial& material = mMaterials[mesh.mMaterialID];
			if(material.mSet) {
				vkCmdBindDescriptorSets(
					aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aLayout, 1, 1, mMaterials[mesh.mMaterialID].mMaterial.GetSet(), 0, nullptr);
			} else {
				ASSERT(false);
			}
		}
		mMesh->QuickTempRender(aBuffer, i);
	}
}

void Model::UpdateMaterials() {
	const int numMaterial = mMesh->GetNumMaterial();
	if(mMaterials.size() < numMaterial) {
		mMaterials.resize(numMaterial);
	}

	for(int i = 0; i < numMaterial; i++) {
		if(mMaterials[i].mSet == false) {
			ModelMaterial& material = mMaterials[i];
			material.mSet			= true;
            material.mMaterial = mMaterialBase->MakeMaterials()[0];
			material.mMaterial.SetImages(*mMesh->GetMaterial(i).mImage, 0, 0);
		}
	}
}