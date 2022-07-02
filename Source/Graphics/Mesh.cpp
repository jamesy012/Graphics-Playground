#include "Mesh.h"

#include "PlatformDebug.h"
#include "Graphics/Conversions.h"

#include "Graphics/Material.h"
#include "Graphics/Image.h"

#include "Loaders/AssimpLoader.h"
#include "Loaders/TinygltfLoader.h"

Job::Work Mesh::GetWork(FileIO::Path aFilePath) {
	const std::string ext = aFilePath.Extension();
	struct LoaderMap {
		MeshLoaders loader;
		std::vector<std::string> extensions;
	};
	MeshLoaders selectedLoader = MeshLoaders::COUNT;

	const LoaderMap loaderMap[(int)MeshLoaders::COUNT] = {{MeshLoaders::TINYGLTF, {".gltf"}}, {}};
	for(int i = 0; i < (int)MeshLoaders::COUNT && selectedLoader == MeshLoaders::COUNT; i++) {
		const LoaderMap& map = loaderMap[i];
		for(int q = 0; q < map.extensions.size(); q++) {
			if(map.extensions[q] == ext) {
				selectedLoader = (MeshLoaders)i;
				break;
			}
		}
	}

	mLoadingBase = nullptr;

	switch(selectedLoader) {
		case MeshLoaders::TINYGLTF:
			mLoadingBase = new TinygltfLoader();
			break;
		case MeshLoaders::ASSIMP:
		case MeshLoaders::COUNT: //fallback
			mLoadingBase = new AssimpLoader();
			break;
		default:
			ASSERT(false);
	}

	if(mLoadingBase == nullptr){
		return Job::Work();
	}

	mLoadingBase->SetUp(this);
	return mLoadingBase->GetWork(aFilePath);
}

bool Mesh::LoadMeshSync(FileIO::Path aFilePath) {
	ZoneScoped;
	ZoneText(aFilePath.String().c_str(), aFilePath.String().size());
	LOGGER::Formated("Loading: {}\n", aFilePath.String());
	LOGGER::Log("Mesh Loading should not load a mesh that is already loaded?\n");

	Job::Work asyncWork = GetWork(aFilePath);
	asyncWork.DoWork();

	return true;
}

bool Mesh::LoadMesh(FileIO::Path aFilePath, FileIO::Path aImagePath /*= ""*/) {
	ZoneScoped;
	ZoneText(aFilePath.String().c_str(), aFilePath.String().size());
	LOGGER::Formated("Loading: {}\n", aFilePath.String());
	LOGGER::Log("Mesh Loading should not load a mesh that is already loaded?\n");

	mImagePath = aImagePath;

	Job::Work asyncWork = GetWork(aFilePath);
	mLoadingHandle		= Job::QueueWorkHandle(asyncWork);

	return true;
}

void Mesh::Destroy() {
	if(mLoadingHandle) {
		Job::WaitForWork(mLoadingHandle);
		mLoadingHandle->Reset();
		mLoadingHandle = nullptr;
	}

	for(int i = 0; i < mMaterials.size(); i++) {
		if(mMaterials[i].mImage) {
			mMaterials[i].mImage->Destroy();
			mMaterials[i].mImage = nullptr;
		}
	}
	for(int i = 0; i < mMesh.size(); i++) {
		SubMesh& mesh = mMesh[i];
		mesh.mVertexBuffer.Destroy();
		mesh.mIndexBuffer.Destroy();
	}
	mMesh.clear();
}

const bool Mesh::HasLoaded() const {
	if(Job::IsDone(mLoadingHandle)) {
		for(auto image: mMaterials) {
			if(image.mImage && !image.mImage->HasLoaded()) {
				return false;
			}
		}
		return true;
	} else {
		//const bool jobDone = Job::IsDone(mLoadingHandle);
		//if(jobDone) {
		//	mLoadingHandle->Reset();
		//	//mLoadingHandle = nullptr;
		//	//try the check again
		//	return HasLoaded();
		//}
	}
	return false;
}

//temp
void Mesh::QuickTempRender(VkCommandBuffer aBuffer, int aMeshIndex) const {
	ZoneScoped;
	if(false == HasLoaded()) {
		return;
	}
	//for(int i = 0; i < mMesh.size(); i++) {
	const SubMesh& mesh = mMesh[aMeshIndex];
	//if(mesh.mMaterial != nullptr) {
	//	vkCmdBindDescriptorSets(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 1, 1, mesh.mMaterial->GetSet(), 0, nullptr);
	//}
	VkDeviceSize offsets[1] = {0};
	vkCmdBindVertexBuffers(aBuffer, 0, 1, mesh.mVertexBuffer.GetBufferRef(), offsets);
	vkCmdBindIndexBuffer(aBuffer, mesh.mIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(aBuffer, mesh.mIndices.size(), 1, 0, 0, 0);
	//}
}
