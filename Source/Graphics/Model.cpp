#include "Model.h"

#include "Graphics.h" //bindless textures

#include "Mesh.h"
#include "PlatformDebug.h"

#include "Image.h"
#include "Graphics/Helpers.h"
#include "Engine/Transform.h"
#include "Graphics/MaterialManager.h"

void Model::Destroy() {
	mLocation.Clear();
}

void Model::SetMaterialBase(MaterialBase* aBase) {
	mMaterialBase = aBase;
}

void Model::SetNumMaterials(int aCount) {
	mMaterials.resize(aCount);
}
void Model::SetMaterial(int aIndex, Material aMaterial) {
	assert(aIndex < mMaterials.size());
	ModelMaterial& material = mMaterials[aIndex];
	material.mMaterial = aMaterial;
	material.mOverride = true;
	material.mSet = true;
}

void Model::Render(VkCommandBuffer aBuffer, VkPipelineLayout aLayout) {
	ZoneScoped;
	if(!mMesh->HasLoaded()) {
		return;
	}
	//UpdateMaterials();
	const int numMesh = mMesh->GetNumMesh();

	for(int i = 0; i < numMesh; i++) {
		const Mesh::SubMesh& mesh = mMesh->GetMesh(i);
		MeshPCTest modelPC;
		if(mesh.mMaterialID != -1) {
			MaterialManager* matManager = gGraphics->GetMaterialManager();
			const Mesh::MeshMaterialData& material = mMesh->GetMaterial(mesh.mMaterialID);

			const auto ApplyTexture = [&matManager](unsigned int& aIndex, Image* aImage, Image* aDefault = CONSTANT::IMAGE::gWhite) {
				if(aImage) {
					matManager->PrepareTexture(aIndex, aImage);
				} else {
					matManager->PrepareTexture(aIndex, aDefault);
				}
			};

			ApplyTexture(modelPC.mAlbedoTexture, material.mImage);
			ApplyTexture(modelPC.mNormalTexture, material.mNormal, CONSTANT::IMAGE::gChecker);
			ApplyTexture(modelPC.mMetallicRoughnessTexture, material.mMetallicRoughnessTexture, CONSTANT::IMAGE::gBlack);
			modelPC.mColorFactor = material.mColorFactor;
			modelPC.mMetallicRoughness = material.mMetallicRoughness;
			modelPC.mNormalBC5 = material.mNormalBC5;

			//
			const VkDescriptorSet* textureSet = matManager->FinializeTextureSet();
			if(textureSet) {
				vkCmdBindDescriptorSets(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aLayout, 1, 1, textureSet, 0, nullptr);
			}
		}
		modelPC.mWorld = mLocation.GetWorldMatrix() * mesh.mMatrix;
		vkCmdPushConstants(aBuffer, aLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(MeshPCTest), &modelPC);
		mMesh->QuickTempRender(aBuffer, i);
	}
}

void Model::UpdateMaterials() {
	const int numMaterial = mMesh->GetNumMaterial();
	if(mMaterials.size() < numMaterial) {
		mMaterials.resize(numMaterial);
	}

	for(int i = 0; i < numMaterial; i++) {
		ModelMaterial& material = mMaterials[i];
		if(material.mMaterial.IsValid() == false) {
			material.mMaterial = mMaterialBase->MakeMaterials()[0];
			material.mMaterial.SetImages(*CONSTANT::IMAGE::gWhite, 0, 0);
		}
		if(mMaterials[i].mSet == false && mMesh->GetMaterial(i).mImage) {
			material.mSet = true;
			material.mMaterial.SetImages(*mMesh->GetMaterial(i).mImage, 0, 0);
		}
	}
}