#pragma once

#include <string>

#include "Mesh/tinygltf/tiny_gltf.h"

#include "LoaderBase.h"
#include "Graphics/Mesh.h"
#include "PlatformDebug.h"
#include "Engine/Transform.h"

class TinygltfLoader : public LoaderBase {
private:
	struct AsyncLoadData {
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
	};

public:
	virtual Job::Work GetWork(FileIO::Path aPath) override;

private:
	void ProcessMaterials(tinygltf::Model& aModel);
	void ProcessModel(tinygltf::Model& aModel);
	void ProcessScene(tinygltf::Model& aModel, tinygltf::Scene& aScene);
	void ProcessNode(tinygltf::Model& aModel, tinygltf::Node& aNode, const glm::mat4& aMatrix);
	void ProcessMesh(tinygltf::Model& aModel, tinygltf::Mesh& aMesh);
	void ProcessPrimitive(tinygltf::Model& aModel, tinygltf::Primitive& aPrimitive);
};

Job::Work TinygltfLoader::GetWork(FileIO::Path aPath) {
	Job::Work work;
	AsyncLoadData* asyncData = new AsyncLoadData();
	work.mUserData = asyncData;
	work.mWorkPtr = [aPath](void* data) {
		ZoneScoped;
		ZoneText(aPath.String().c_str(), aPath.String().size());
		AsyncLoadData* asyncData = (AsyncLoadData*)data;
		std::string err;
		std::string warn;

		asyncData->loader.SetStoreOriginalJSONForExtrasAndExtensions(true);

		asyncData->loader.LoadASCIIFromFile(&asyncData->model, &err, &warn, aPath.String());

		if(!err.empty()) {
			LOGGER::Formated("gltf Load error {}\n\t{}", aPath.String(), err);
		}
		if(!warn.empty()) {
			LOGGER::Formated("gltf Load warnings {}\n\t{}", aPath.String(), warn);
		}
	};
	work.mFinishPtr = [this](void* data) {
		ZoneScoped;
		AsyncLoadData* asyncData = (AsyncLoadData*)data;

		ProcessMaterials(asyncData->model);
		ProcessModel(asyncData->model);
		delete asyncData;
	};

	return work;
}

void TinygltfLoader::ProcessMaterials(tinygltf::Model& aModel) {
	ZoneScoped;
	const size_t numMaterials = aModel.materials.size();
	mMesh->mMaterials.resize(numMaterials);
	for(size_t i = 0; i < numMaterials; i++) {
		ZoneScoped;
		Mesh::MeshMaterialData& materialData = mMesh->mMaterials[i];
		tinygltf::Material& mat = aModel.materials[i];
		auto SetLoadTexture = [&](tinygltf::Image& aTinyImagePath, Image** aOutputImage) {
			LOGGER::Formated("Loading Texture {}\n", aTinyImagePath.uri);
			Image* image = new Image();
			image->LoadImage(mMesh->mImagePath + aTinyImagePath.uri, VK_FORMAT_UNDEFINED);
			*aOutputImage = image;
		};
		if(mat.pbrMetallicRoughness.baseColorTexture.index != -1) {
			tinygltf::Texture& baseTexture = aModel.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
			SetLoadTexture(aModel.images[baseTexture.source], &materialData.mImage);
		}
		if(mat.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
			tinygltf::Texture& baseTexture = aModel.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
			SetLoadTexture(aModel.images[baseTexture.source], &materialData.mMetallicRoughnessTexture);
		}
		if(mat.normalTexture.index != -1) {
			tinygltf::Texture& baseTexture = aModel.textures[mat.normalTexture.index];
			SetLoadTexture(aModel.images[baseTexture.source], &materialData.mNormal);
		}
		for(int i = 0; i < mat.pbrMetallicRoughness.baseColorFactor.size(); i++) {
			materialData.mColorFactor[i] = mat.pbrMetallicRoughness.baseColorFactor[i];
		}
		materialData.mMetallicRoughness.x = mat.pbrMetallicRoughness.metallicFactor;
		materialData.mMetallicRoughness.y = mat.pbrMetallicRoughness.roughnessFactor;
		materialData.mAlphaCutoff = mat.alphaCutoff;
	}
}

void TinygltfLoader::ProcessModel(tinygltf::Model& aModel) {
	ZoneScoped;
	//const size_t numScenes = aModel.scenes.size();
	//for(size_t i = 0; i < numScenes; i++) {
	//	ProcessScene(aModel, aModel.scenes[i]);
	//}
	ASSERT(aModel.scenes.size() == 1);
	ASSERT(aModel.defaultScene == 0);
	ProcessScene(aModel, aModel.scenes[aModel.defaultScene]);
}
void TinygltfLoader::ProcessScene(tinygltf::Model& aModel, tinygltf::Scene& aScene) {
	const int numNodes = aScene.nodes.size();
	for(size_t i = 0; i < numNodes; i++) {
		ProcessNode(aModel, aModel.nodes[aScene.nodes[i]], glm::mat4(1));
	}
}
void TinygltfLoader::ProcessNode(tinygltf::Model& aModel, tinygltf::Node& aNode, const glm::mat4& aMatrix) {

	//also contains position information here
	SimpleTransform nodeTransform;
	ASSERT(aNode.matrix.size() == 0 || aNode.matrix.size() == 16);
	ASSERT(aNode.translation.size() == 3 || aNode.translation.size() == 0);
	ASSERT(aNode.scale.size() == 3 || aNode.scale.size() == 0);
	ASSERT(aNode.rotation.size() == 3 || aNode.rotation.size() == 4 || aNode.rotation.size() == 0);
	if(aNode.translation.size() == 3) {
		nodeTransform.SetPosition(glm::vec3(aNode.translation[0], aNode.translation[1], aNode.translation[2]));
	}
	if(aNode.scale.size() == 3) {
		nodeTransform.SetScale(glm::vec3(aNode.scale[0], aNode.scale[1], aNode.scale[2]));
	}
	if(aNode.rotation.size() == 3) {
		nodeTransform.SetRotation(glm::vec3(aNode.rotation[0], aNode.rotation[1], aNode.rotation[2]));
	} else if(aNode.rotation.size() == 4) {
		nodeTransform.SetRotation(glm::quat(aNode.rotation[3], aNode.rotation[0], aNode.rotation[1], aNode.rotation[2]));
	}
	if(aNode.matrix.size() == 16) {
		// clang-format off
		glm::mat4 mat(aNode.matrix[0],			  aNode.matrix[1],				  aNode.matrix[2],				  aNode.matrix[3],
				  aNode.matrix[4],				  aNode.matrix[5],				  aNode.matrix[6],				  aNode.matrix[7],
				  aNode.matrix[8],				  aNode.matrix[9],				  aNode.matrix[10],				  aNode.matrix[11],
				  aNode.matrix[12],				  aNode.matrix[13],				  aNode.matrix[14],				  aNode.matrix[15]);
		// clang-format on
		nodeTransform.SetMatrix(mat);
	}
	const glm::mat4 nodeMatrix = aMatrix * nodeTransform.GetLocalMatrix();

	const int numChildren = aNode.children.size();
	if(numChildren != 0) {
		for(size_t i = 0; i < numChildren; i++) {
			ProcessNode(aModel, aModel.nodes[aNode.children[i]], nodeMatrix);
		}
	}
	if(aNode.mesh == -1) {
		return;
	}
	const int numMeshBefore = mMesh->mMesh.size();
	ProcessMesh(aModel, aModel.meshes[aNode.mesh]);
	const int numMeshs = mMesh->mMesh.size();
	for(size_t i = numMeshBefore; i < numMeshs; i++) {
		mMesh->mMesh[i].mMatrix = nodeMatrix;
	}
}
void TinygltfLoader::ProcessMesh(tinygltf::Model& aModel, tinygltf::Mesh& aMesh) {
	const int numPrimitives = aMesh.primitives.size();
	for(size_t i = 0; i < numPrimitives; i++) {
		ProcessPrimitive(aModel, aMesh.primitives[i]);
	}
}
void TinygltfLoader::ProcessPrimitive(tinygltf::Model& aModel, tinygltf::Primitive& aPrimitive) {
	ZoneScoped;

	ASSERT(aPrimitive.mode == TINYGLTF_MODE_TRIANGLES);

	mMesh->mMesh.push_back(Mesh::SubMesh());
	Mesh::SubMesh& mesh = mMesh->mMesh.back();
	std::vector<MeshVert>& vertices = mesh.mVertices;
	std::vector<MeshIndex>& indices = mesh.mIndices;

	int vertCount = -1;
	auto SetOrValidateVertCount = [&vertCount, &vertices](int count) {
		if(vertCount == -1) {
			vertices.resize(count);
			vertCount = count;
			return;
		}
		ASSERT(vertCount == count);
	};

	mesh.mMaterialID = aPrimitive.material;

	//vertex
	//POSITION, NORMAL, TANGENT, TEXCOORD_n, COLOR_n, JOINTS_n
	//https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#meshes
	if(aPrimitive.attributes.contains("POSITION")) {
		const tinygltf::Accessor& accessor = aModel.accessors[aPrimitive.attributes["POSITION"]];
		const tinygltf::BufferView& view = aModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = aModel.buffers[view.buffer];
		const unsigned char* datastart = &buffer.data[accessor.byteOffset + view.byteOffset];
		const int dataSize =
			accessor.ByteStride(view); //tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
		const int count = accessor.count;

		ASSERT(accessor.type == TINYGLTF_TYPE_VEC3 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
		SetOrValidateVertCount(count);

		if(accessor.minValues.size() == 3 && accessor.maxValues.size() == 3) {
			mesh.mAABB.mMin = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
			mesh.mAABB.mMax = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
		}

		for(int i = 0; i < count; i++) {
			glm::vec3 data = glm::vec3(0);
			memcpy(&data, datastart + (dataSize * i), dataSize * sizeof(char));
			vertices[i].mPos = data;
		}
	}
	if(aPrimitive.attributes.contains("NORMAL")) {
		const tinygltf::Accessor& accessor = aModel.accessors[aPrimitive.attributes["NORMAL"]];
		const tinygltf::BufferView& view = aModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = aModel.buffers[view.buffer];
		const unsigned char* datastart = &buffer.data[accessor.byteOffset + view.byteOffset];
		const int dataSize =
			accessor.ByteStride(view); //tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
		const int count = accessor.count;

		ASSERT(accessor.type == TINYGLTF_TYPE_VEC3 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
		SetOrValidateVertCount(count);

		for(int i = 0; i < count; i++) {
			glm::vec3 data = glm::vec3(0);
			memcpy(&data, datastart + (dataSize * i), dataSize * sizeof(char));
			vertices[i].mNorm = data;
		}
	}
	if(aPrimitive.attributes.contains("TANGENT")) {
		const tinygltf::Accessor& accessor = aModel.accessors[aPrimitive.attributes["TANGENT"]];
		const tinygltf::BufferView& view = aModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = aModel.buffers[view.buffer];
		const unsigned char* datastart = &buffer.data[accessor.byteOffset + view.byteOffset];
		const int dataSize =
			accessor.ByteStride(view); //tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
		const int count = accessor.count;

		ASSERT(accessor.type == TINYGLTF_TYPE_VEC4 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
		SetOrValidateVertCount(count);

		for(int i = 0; i < count; i++) {
			glm::vec4 data = glm::vec4(0);
			memcpy(&data, datastart + (dataSize * i), dataSize * sizeof(char));
			vertices[i].mTangent = glm::vec3(data);
		}
	}
	if(aPrimitive.attributes.contains("TEXCOORD_0")) {
		const tinygltf::Accessor& accessor = aModel.accessors[aPrimitive.attributes["TEXCOORD_0"]];
		const tinygltf::BufferView& view = aModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = aModel.buffers[view.buffer];
		const unsigned char* datastart = &buffer.data[accessor.byteOffset + view.byteOffset];
		const int dataSize =
			accessor.ByteStride(view); //tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
		const int count = accessor.count;

		ASSERT(accessor.type == TINYGLTF_TYPE_VEC2 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
		SetOrValidateVertCount(count);

		for(int i = 0; i < count; i++) {
			glm::vec2 data = glm::vec2(0);
			memcpy(&data, datastart + (dataSize * i), dataSize * sizeof(char));
			vertices[i].mUVs[0] = data;
		}
	}
	if(aPrimitive.attributes.contains("COLOR_0")) {
		const tinygltf::Accessor& accessor = aModel.accessors[aPrimitive.attributes["COLOR_0"]];
		const tinygltf::BufferView& view = aModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = aModel.buffers[view.buffer];
		const unsigned char* dataStart = &buffer.data[accessor.byteOffset + view.byteOffset];
		const int dataSize =
			accessor.ByteStride(view); //tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
		const int count = accessor.count;

		ASSERT(accessor.type == TINYGLTF_TYPE_VEC4 &&
			   (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT));
		SetOrValidateVertCount(count);

		for(int i = 0; i < count; i++) {
			glm::vec4 data = glm::vec4(0);
			switch(accessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					unsigned short tempData[4];
					memcpy(&tempData, dataStart + (dataSize * i), dataSize);
					data[0] = (float)tempData[0] / USHRT_MAX;
					data[1] = (float)tempData[1] / USHRT_MAX;
					data[2] = (float)tempData[2] / USHRT_MAX;
					data[3] = (float)tempData[3] / USHRT_MAX;
					break;
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					memcpy(&data, dataStart + (dataSize * i), dataSize * sizeof(float));
					break;
				default:
					ASSERT(false);
					break;
			}

			vertices[i].mColors[0] = data;
		}
	} else {
		ASSERT(vertCount != -1);
		for(int i = 0; i < vertCount; i++) {
			glm::vec4 data = glm::vec4(1);
			vertices[i].mColors[0] = data;
		}
	}

	//index
	{
		const tinygltf::Accessor& accessor = aModel.accessors[aPrimitive.indices];
		const tinygltf::BufferView& view = aModel.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = aModel.buffers[view.buffer];
		const unsigned char* datastart = &buffer.data[accessor.byteOffset + view.byteOffset];
		const int dataSize =
			accessor.ByteStride(view); //tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);
		const int count = accessor.count;

		ASSERT(accessor.type == TINYGLTF_TYPE_SCALAR);
		indices.resize(count);

		for(int i = 0; i < count; i++) {
			MeshIndex data = 0;
			memcpy(&data, datastart + (dataSize * i), dataSize);
			indices[i] = data;
		}
	}

	//temp
	mesh.mVertexBuffer.CreateFromData(BufferType::VERTEX, sizeof(MeshVert) * vertices.size(), vertices.data(), "Mesh Vertex Data");
	mesh.mIndexBuffer.CreateFromData(BufferType::INDEX, sizeof(MeshIndex) * indices.size(), indices.data(), "Mesh Index Data");
	mesh.mVertexBuffer.Flush();
	mesh.mIndexBuffer.Flush();
}
