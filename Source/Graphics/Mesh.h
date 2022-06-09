#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Engine/FileIO.h"
#include "Engine/AABB.h"

#include "Buffer.h"

struct aiScene;
struct aiNode;
struct aiMesh;

static constexpr char NUM_UVS		= 8;
static constexpr char NUM_VERT_COLS = 8;

//includes all per mesh data we can load from assimp
struct MeshVert {
	glm::vec3 mPos;
	glm::vec3 mNorm;
	glm::vec3 mTangent;
	glm::vec3 mBiTangent;
	glm::vec4 mColor[NUM_VERT_COLS];
	glm::vec2 mUV[NUM_UVS];
};
typedef uint32_t MeshIndex;

class Mesh {
public:
	bool LoadMesh(FileIO::Path aFilePath);
    void Destroy();

	struct SubMesh {
		std::vector<MeshVert> mVertices;
		std::vector<MeshIndex> mIndices;
		AABB mAABB;

        Buffer mVertexBuffer;
        Buffer mIndexBuffer;
	};

    void QuickTempRender(VkCommandBuffer aBuffer);

	uint32_t GetNumMesh() const {
		return mMesh.size();
	}

	const SubMesh& GetMesh(uint32_t mesh) const {
		return mMesh[mesh];
	}

private:
	bool ProcessNode(const aiScene* aScene, const aiNode* aNode);
	bool ProcessMesh(const aiScene* aScene, const aiMesh* aMesh);

	AABB mAABB;
	std::vector<SubMesh> mMesh;
};