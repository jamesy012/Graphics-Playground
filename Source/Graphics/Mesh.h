#pragma once

#include <glm/glm.hpp>

#include "Engine/FileIO.h"

class aiScene;
class aiNode;
class aiMesh;

static constexpr char NUM_UVS = 8;
static constexpr char NUM_VERT_COLS = 8;

class Mesh {
public:
	bool LoadMesh(FileIO::Path aFilePath);

private:
	bool ProcessNode(const aiScene* aScene, const aiNode* aNode);
	bool ProcessMesh(const aiScene* aScene, const aiMesh* aMesh);

    struct MeshVert{
        glm::vec3 mPos;
        glm::vec3 mNorm;
        glm::vec3 mTangent;
        glm::vec3 mBiTangent;
        glm::vec4 mColor[NUM_VERT_COLS];
        glm::vec2 mUV[NUM_UVS];
    };
};