#pragma once

#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#include "LoaderBase.h"
#include "Graphics/Mesh.h"

#include "Graphics/Image.h"

#include "PlatformDebug.h"
#include "Graphics/Conversions.h"

static_assert(NUM_UVS <= AI_MAX_NUMBER_OF_TEXTURECOORDS);
static_assert(NUM_VERT_COLS <= AI_MAX_NUMBER_OF_COLOR_SETS);

class AssimpLoader : public LoaderBase {
private:
	struct AsyncLoadData {
		const aiScene* mScene;
		Assimp::Importer importer;
	};

public:
	virtual Job::Work GetWork(FileIO::Path aPath) override;

private:
	bool ProcessNode(const aiScene* aScene, const aiNode* aNode);
	bool ProcessMesh(const aiScene* aScene, const aiMesh* aMesh);
};

Job::Work AssimpLoader::GetWork(FileIO::Path aPath) {
	Job::Work asyncWork;
	asyncWork.mUserData = new AsyncLoadData();
	asyncWork.mWorkPtr	= [aPath](void* aData) {
		 ZoneScoped;
		 ZoneText(aPath.String().c_str(), aPath.String().size());
		 AsyncLoadData* data  = (AsyncLoadData*)aData;
		 const aiScene* scene = data->importer.ReadFile(aPath.String().c_str(),
														aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
															aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_OptimizeMeshes |
															aiProcess_OptimizeGraph | aiProcess_GenBoundingBoxes);

		 if(scene == nullptr) {
			 LOGGER::Formated("Failed to load Model {}\n", aPath.String());
			 //ASSERT(false);
			 //return false;
			 return;
		 }
		 data->mScene = scene;
	};
	//asyncWork.mFinishOnMainThread = true;
	asyncWork.mFinishPtr = [this](void* aData) {
		ZoneScoped;
		AsyncLoadData* data = (AsyncLoadData*)aData;
		if(data->mScene) {
			mMesh->mMaterials.resize(data->mScene->mNumMaterials);
			ProcessNode(data->mScene, data->mScene->mRootNode);
			data->importer.FreeScene();
		}
		//we create this for the work, lets delete it now
		delete data;
	};
	return asyncWork;
};

bool AssimpLoader::ProcessNode(const aiScene* aScene, const aiNode* aNode) {
	ZoneScoped;
	for(uint16_t i = 0; i < aNode->mNumMeshes; i++) {
		int meshId = aNode->mMeshes[aNode->mMeshes[i]];
		//meshes_.push_back(this->processMesh(aScene, mesh));
		this->ProcessMesh(aScene, aScene->mMeshes[meshId]);
	}
	for(uint16_t i = 0; i < aNode->mNumChildren; i++) {
		this->ProcessNode(aScene, aNode->mChildren[i]);
	}
	return true;
}

bool AssimpLoader::ProcessMesh(const aiScene* aScene, const aiMesh* aMesh) {
	ZoneScoped;
	mMesh->mMesh.push_back(Mesh::SubMesh());
	Mesh::SubMesh& mesh				= mMesh->mMesh.back();
	std::vector<MeshVert>& vertices = mesh.mVertices;
	std::vector<MeshIndex>& indices = mesh.mIndices;

	//todo memcpy these values?

	mesh.mVertices.resize(aMesh->mNumVertices);
	//Vertices
	for(uint64_t i = 0; i < aMesh->mNumVertices; i++) {
		MeshVert vertex;
		if(aMesh->HasPositions()) {
			vertex.mPos = AssimpToGlm(aMesh->mVertices[i]);
		}

		if(aMesh->HasNormals()) {
			vertex.mNorm = AssimpToGlm(aMesh->mNormals[i]);
		}

		if(aMesh->HasTangentsAndBitangents()) {
			vertex.mTangent	  = AssimpToGlm(aMesh->mTangents[i]);
			vertex.mBiTangent = AssimpToGlm(aMesh->mBitangents[i]);
		}

		for(uint64_t channel = 0; channel < NUM_VERT_COLS; channel++) {
			if(aMesh->HasVertexColors(channel)) {
				vertex.mColors[channel] = AssimpToGlm(aMesh->mColors[channel][i]);
			} else {
				vertex.mColors[channel] = glm::vec4(1, 0, 1, 1);
			}
		}

		for(uint64_t channel = 0; channel < NUM_UVS; channel++) {
			if(aMesh->HasTextureCoords(channel)) {
				vertex.mUVs[channel] = AssimpToGlm(aMesh->mTextureCoords[channel][i]);
			} else {
				vertex.mUVs[channel] = glm::vec2(-1);
			}
		}

		vertices[i] = vertex;
	}

	//Indices
	{
		//is the count here useful?
		uint64_t indicesCount = 0;
		for(uint64_t i = 0; i < aMesh->mNumFaces; i++) {
			const aiFace& face = aMesh->mFaces[i];
			indicesCount += face.mNumIndices;
		}
		indices.resize(indicesCount);
		indicesCount = 0;
		for(uint64_t i = 0; i < aMesh->mNumFaces; i++) {
			const aiFace& face = aMesh->mFaces[i];

			for(uint64_t j = 0; j < face.mNumIndices; j++) {
				indices[indicesCount + j] = (MeshIndex)face.mIndices[j];
			}
			indicesCount += face.mNumIndices;
		}
	}

	//todo animation
	//Bones
	for(uint16_t i = 0; i < aMesh->mNumBones; i++) {
		const aiBone* bone = aMesh->mBones[i];
		ASSERT(false);
	}

	//Animations/per-vertex animations
	for(uint16_t i = 0; i < aMesh->mNumAnimMeshes; i++) {
		const aiAnimMesh* anim = aMesh->mAnimMeshes[i];
		ASSERT(false);
	}

	//AABB
	{
		const aiAABB& aabb = aMesh->mAABB;
		mesh.mAABB.Expand(AssimpToGlm(aabb.mMin), AssimpToGlm(aabb.mMax));
		mMesh->mAABB.Expand(mesh.mAABB);
	}

	//Material
	if(aMesh->mMaterialIndex >= 0) {
		mesh.mMaterialID		   = aMesh->mMaterialIndex;
		const aiMaterial* material = aScene->mMaterials[aMesh->mMaterialIndex];
		//for(int i = 0; i < material->mNumProperties; i++) {
		//	const aiMaterialProperty* prop = material->mProperties[i];
		//	//if(strcmp(prop->mKey.data, _AI_MATKEY_TEXTURE_BASE)){
		//	//LOGGER::Formated("Material semantic {}, {}\n", prop->mKey.C_Str(), prop->mSemantic);
		//	//}
		//}
		for(unsigned int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
			aiString str;
			material->GetTexture(aiTextureType_DIFFUSE, i, &str);
			const std::string fileName = str.C_Str();
			//bool loaded				   = false;
			//for(size_t q = 0; q < mMaterials.size(); q++) {
			//	if(mMaterials[q].mFileName == fileName) {
			//		mesh.mMaterialID = aMesh->mMaterialIndex;
			//		loaded			 = true;
			//	}
			//}
			//if(loaded) {
			//	continue;
			//}
			Mesh::MeshMaterialData& materialData = mMesh->mMaterials[aMesh->mMaterialIndex];
			if(materialData.mImage == nullptr) {
				Image* image = new Image();
				image->LoadImage(mMesh->mImagePath + fileName, VK_FORMAT_UNDEFINED);
				materialData.mFileName = fileName;
				materialData.mImage	   = image;
			}
		}

		//std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
	}

	//temp
	mesh.mVertexBuffer.CreateFromData(BufferType::VERTEX, sizeof(MeshVert) * vertices.size(), vertices.data(), "Mesh Vertex Data");
	mesh.mIndexBuffer.CreateFromData(BufferType::INDEX, sizeof(MeshIndex) * indices.size(), indices.data(), "Mesh Index Data");
	mesh.mVertexBuffer.Flush();
	mesh.mIndexBuffer.Flush();

	return true;
}