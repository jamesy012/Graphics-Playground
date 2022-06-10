#include "Mesh.h"

#include <assimp/Importer.hpp> // C++ importer interface
#include <assimp/scene.h> // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#include "PlatformDebug.h"
#include "Conversions.h"

static_assert(NUM_UVS <= AI_MAX_NUMBER_OF_TEXTURECOORDS);
static_assert(NUM_VERT_COLS <= AI_MAX_NUMBER_OF_COLOR_SETS);

bool Mesh::LoadMesh(FileIO::Path aFilePath) {
	LOG::Log("Loading: %s\n", aFilePath.mPath.c_str());
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

	importer.FreeScene();
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
	mMesh.push_back(SubMesh());
	SubMesh& mesh					= mMesh.back();
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
				vertex.mColor[channel] = AssimpToGlm(*aMesh->mColors[i]);
			} else {
				vertex.mColor[channel] = glm::vec4(1, 0, 1, 1);
			}
		}

		for(uint64_t channel = 0; channel < NUM_UVS; channel++) {
			if(aMesh->HasTextureCoords(channel)) {
				vertex.mUV[channel] = AssimpToGlm(aMesh->mTextureCoords[channel][i]);
			} else {
				vertex.mUV[channel] = glm::vec2(-1);
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
		mAABB.Expand(mesh.mAABB);
	}

	//Material
	if(aMesh->mMaterialIndex >= 0) {
		const aiMaterial* material = aScene->mMaterials[aMesh->mMaterialIndex];

		//std::vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
	}

	//temp
	mesh.mVertexBuffer.CreateFromData(BufferType::VERTEX, sizeof(MeshVert) * vertices.size(), vertices.data(), "Mesh Vertex Data");
	mesh.mIndexBuffer.CreateFromData(BufferType::INDEX, sizeof(MeshIndex) * indices.size(), indices.data(), "Mesh Vertex Data");
	mesh.mVertexBuffer.Flush();
	mesh.mIndexBuffer.Flush();

	return true;
}

void Mesh::Destroy() {
	for(int i = 0; i < mMesh.size(); i++) {
		SubMesh& mesh = mMesh[i];
		mesh.mVertexBuffer.Destroy();
		mesh.mIndexBuffer.Destroy();
	}
	mMesh.clear();
}

//temp
void Mesh::QuickTempRender(VkCommandBuffer aBuffer) {
	for(int i = 0; i < mMesh.size(); i++) {
		SubMesh& mesh			= mMesh[i];
		VkDeviceSize offsets[1] = {0};
		vkCmdBindVertexBuffers(aBuffer, 0, 1, mesh.mVertexBuffer.GetBufferRef(), offsets);
		vkCmdBindIndexBuffer(aBuffer, mesh.mIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(aBuffer, mesh.mIndices.size(), 1, 0, 0, 0);
	}
}