#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Engine/FileIO.h"
#include "Engine/AABB.h"
//#include "Engine/Transform.h"

#include "Buffer.h"
#include "Engine/Job.h"

class Image;
class Model;
class LoaderBase;
class AssimpLoader;
class TinygltfLoader;

static constexpr char NUM_UVS		= 1;
static constexpr char NUM_VERT_COLS = 1;

//includes all per mesh data we can load from assimp
struct MeshVert {
	glm::vec3 mPos;
	glm::vec3 mNorm;
	glm::vec3 mTangent;
	glm::vec3 mBiTangent;
	glm::vec4 mColors[NUM_VERT_COLS];
	glm::vec2 mUVs[NUM_UVS];
};
typedef uint32_t MeshIndex;


class Mesh {
public:
	friend Model;
	friend AssimpLoader;
	friend TinygltfLoader;
	bool LoadMeshSync(FileIO::Path aFilePath, FileIO::Path aImagePath = "");
	bool LoadMesh(FileIO::Path aFilePath, FileIO::Path aImagePath = "");
	enum Shapes{
		BOX
	};
	bool LoadShape(Shapes aShape);
	void Destroy();

	struct MeshMaterialData {
		Image* mImage = nullptr;
		Image* mNormal = nullptr;
		Image* mMetallicRoughnessTexture = nullptr;
		float mAlphaCutoff = 0.5f;
		glm::vec4 mColorFactor = glm::vec4(1.0f);
		glm::vec2 mMetallicRoughness = glm::vec2(1.0f);
		bool mNormalBC5 = false;//bc5 textures are incorrect atm
	};

	struct SubMesh {
		std::vector<MeshVert> mVertices;
		std::vector<MeshIndex> mIndices;
		AABB mAABB;

		glm::mat4 mMatrix = glm::mat4(1);
		//SimpleTransform mTransform;//should this be a normal Transform?

		Buffer mVertexBuffer;
		Buffer mIndexBuffer;

		int mMaterialID = -1;
	};

	uint32_t GetNumMesh() const {
		return mMesh.size();
	}
	const SubMesh& GetMesh(uint32_t aMeshIndex) const {
		return mMesh[aMeshIndex];
	}
	uint32_t GetNumMaterial() const {
		return mMaterials.size();
	}
	const MeshMaterialData& GetMaterial(uint32_t aMaterialIndex) const {
		return mMaterials[aMaterialIndex];
	}

	//checks if this mesh and it's images are loaded
	const bool HasLoaded() const;

protected:
    void QuickTempRender(VkCommandBuffer aBuffer, int aMeshIndex) const;

private:

	Job::Work GetWork(FileIO::Path aFilePath);

	AABB mAABB;
	std::vector<SubMesh> mMesh;
	std::vector<MeshMaterialData> mMaterials;

	Job::WorkHandle* mLoadingHandle = nullptr;
	LoaderBase* mLoadingBase = nullptr;

	std::string mImagePath;
};