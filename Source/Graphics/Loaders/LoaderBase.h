#pragma once

#include "Engine/Job.h"
#include "Engine/FileIO.h"

class Mesh;
class Image;

enum class MeshLoaders {
	TINYGLTF,
	ASSIMP,
	COUNT
};

class LoaderBase {
public:
	virtual Job::Work GetWork(FileIO::Path aPath) = 0;

	void SetUp(Mesh* aMesh) {
		mMesh = aMesh;
	}
	void SetUp(Image* aImage) {
		mImage = aImage;
	}

protected:
	union {
		Mesh* mMesh;
		Image* mImage;
	};
};