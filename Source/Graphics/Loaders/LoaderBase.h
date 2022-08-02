#pragma once

#include "Engine/Job.h"
#include "Engine/FileIO.h"

class Mesh;
class Image;

enum class MeshLoaders
{
	TINYGLTF, //for .gltf files
	ASSIMP, //generic fallback
	COUNT
};

enum class ImageLoaders
{
	DDS, //for .dds files
	STB, //generic fallback
	COUNT
};

class LoaderBase {
public:
	virtual ~LoaderBase() {
		//
	}

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