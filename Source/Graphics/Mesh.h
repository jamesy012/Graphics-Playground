#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Engine/FileIO.h"
#include "Engine/AABB.h"

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
	bool LoadMeshSync(FileIO::Path aFilePath);
	bool LoadMesh(FileIO::Path aFilePath, FileIO::Path aImagePath = "");
	void Destroy();

	struct MeshMaterialData {
		std::string mFileName;
		Image* mImage = nullptr;
	};

	struct SubMesh {
		std::vector<MeshVert> mVertices;
		std::vector<MeshIndex> mIndices;
		AABB mAABB;

		Buffer mVertexBuffer;
		Buffer mIndexBuffer;

		int mMaterialID;
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

	Job::WorkHandle* mLoadingHandle;
	LoaderBase* mLoadingBase;

	std::string mImagePath;
};