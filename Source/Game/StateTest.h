#pragma once

#include "Engine/StateBase.h"

#include "Graphics/Helpers.h"

#include "Graphics/Material.h"
#include "Engine/Transform.h"
#include "Engine/Camera/FlyCamera.h"

class RenderPass;
class Mesh;
class Buffer;
class Image;
class Framebuffer;
class Pipeline;
class Model;
class Screenspace;

class StateTest : public StateBase {
public:
	void Initalize() override;
	void StartUp() override;
	void ImGuiRender() override;
	void Update() override;
	void Render() override;
	void Finish() override;

private:
	void ChangeMesh(int aIndex);

	RenderPass* mainRenderPass;
	Buffer* sceneDataBuffer;

	Image* fbImage;
	Image* fbDepthImage;
	Framebuffer* fb;
	Pipeline* meshPipeline;

	Mesh* meshSceneTest;
	Mesh* meshTest;
	Mesh* handMesh;
	Mesh* referenceMesh;

	Model* modelSceneTest;
	Model* modelTest1;
	Model* modelTest2;
	Model* controllerTest1;
	Model* controllerTest2;
	Model* worldBase;

	MaterialBase meshTestBase;
	Material meshMaterial;
	MaterialBase ssTestBase;
	Screenspace* ssTest;

	Transform mRootTransform;
	FlyCamera camera;

	SceneData sceneData;
	int selectedMesh = 0;
};
