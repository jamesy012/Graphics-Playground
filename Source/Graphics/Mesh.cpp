#include "Mesh.h"

#include <vector>

#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#include "PlatformDebug.h"
#include "Conversions.h"

static_assert(NUM_UVS == AI_MAX_NUMBER_OF_TEXTURECOORDS);
static_assert(NUM_VERT_COLS == AI_MAX_NUMBER_OF_COLOR_SETS);

bool Mesh::LoadMesh(FileIO::Path aFilePath) {
	LOG::LogLine("Loading: %s", aFilePath.mPath.c_str());
	LOG::LogLine("Mesh Loading should not load a mesh that is already loaded?");

	Assimp::Importer importer;
	const aiScene* scene =
		importer.ReadFile(aFilePath.mPath.c_str(),
						  aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals |
							  aiProcess_FlipUVs | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_GenBoundingBoxes);

	if(scene == nullptr) {
		ASSERT(false);
		return false;
	}

	ProcessNode(scene, scene->mRootNode);

	return true;
}

bool Mesh::ProcessNode(const aiScene* aScene, const aiNode* aNode) {
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

bool Mesh::ProcessMesh(const aiScene* aScene, const aiMesh* aMesh) {
	std::vector<MeshVert> vertices;
	std::vector<uint64_t> indices;

	//todo memcpy these values?

	// Walk through each of the mesh's vertices
	for(uint64_t i = 0; i < aMesh->mNumVertices; i++) {
		MeshVert vertex;
		if(aMesh->HasPositions()) {
			vertex.mPos = AssimpToGlm(aMesh->mVertices[i]);
		}

		if(aMesh->HasNormals()) {
			vertex.mNorm = AssimpToGlm(aMesh->mNormals[i]);
		}

		if(aMesh->HasTangentsAndBitangents()) {
			vertex.mTangent = AssimpToGlm(aMesh->mTangents[i]);
			vertex.mBiTangent = AssimpToGlm(aMesh->mBitangents[i]);
		}

		for(uint64_t channel = 0; channel < NUM_VERT_COLS; channel++) {
			if(aMesh->HasVertexColors(channel)) {
				vertex.mColor[channel] = AssimpToGlm(*aMesh->mColors[i]);
			} else {
				vertex.mColor[channel] = glm::vec4(0, 0, 0, 1);
			}
		}

		for(uint64_t channel = 0; channel < NUM_UVS; channel++) {
			if(aMesh->HasTextureCoords(channel)) {
				vertex.mUV[channel] = AssimpToGlm(aMesh->mTextureCoords[channel][i]);
			} else {
				vertex.mUV[channel] = glm::vec2(-1);
			}
		}

		vertices.push_back(vertex);
	}

	//indices
	for(uint64_t i = 0; i < aMesh->mNumFaces; i++) {
		const aiFace& face = aMesh->mFaces[i];

		for(uint64_t j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

    //animation todo
	//bones
	for(uint16_t i = 0; i < aMesh->mNumBones; i++) {
		const aiBone* bone = aMesh->mBones[i];
	}

	//animations
	for(uint16_t i = 0; i < aMesh->mNumAnimMeshes; i++) {
		const aiAnimMesh* anim = aMesh->mAnimMeshes[i];
	}

	//aabb
	{
		const aiAABB& aabb = aMesh->mAABB;
		aabb.mMin;
		aabb.mMax;
	}

	if(aMesh->mMaterialIndex >= 0) {
		aiMaterial* material = aScene->mMaterials[aMesh->mMaterialIndex];

		//std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
	}

	return true;
}
