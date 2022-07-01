#pragma once

#include <string>

#include "Mesh/tinygltf/tiny_gltf.h"

#include "LoaderBase.h"
#include "Graphics/Mesh.h"
#include "PlatformDebug.h"

class TinygltfLoader : public LoaderBase {
private:
	struct AsyncLoadData {};

public:
	virtual Job::Work GetWork(FileIO::Path aPath) override;

private:
};

Job::Work TinygltfLoader::GetWork(FileIO::Path aPath) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	loader.LoadASCIIFromFile(&model, &err, &warn, aPath.String());
    
	if(!err.empty()) {
		LOGGER::Formated("gltf Load error {}\n\t{}", aPath.String(), err);
	}
	if(!warn.empty()) {
		LOGGER::Formated("gltf Load warnings {}\n\t{}", aPath.String(), warn);
	}

	//todo process

	return Job::Work();
}
