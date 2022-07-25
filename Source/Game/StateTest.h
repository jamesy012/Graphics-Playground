#pragma once

#include "Engine/StateBase.h"

#include "Graphics/Helpers.h"

#include "Graphics/Material.h"
#include "Engine/Transform.h"
#include "Engine/Camera/FlyCamera.h"
#include "Engine/PhysicsObject.h"

class RenderPass;
class Mesh;
class Buffer;
class Image;
class Framebuffer;
class Pipeline;
class Model;
class PhysicsObject;
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
	void SetupPhysicsObjects();

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
	Mesh* physicsMesh;

	Model* modelSceneTest;
	Model* modelTest1;
	Model* modelTest2;
	Model* controllerTest1;
	Model* controllerTest2;
	Model* worldBase;
	static const int numPhysicsObjects = 25;
	Model* physicsModels[numPhysicsObjects];
	PhysicsObject physicsObjects[numPhysicsObjects];

	MaterialBase meshTestBase;
	Material meshMaterial;
	MaterialBase ssTestBase;
	Screenspace* ssTest;

	Transform mRootTransform;
	FlyCamera camera;

    PhysicsObject mGroundPlane;
    Transform mGroundTransform;

	SceneData sceneData;
	int selectedMesh = 0;

	bool mRenderTrees = false;

#if defined(ENABLE_XR)
	Screenspace* vrMirrorPass;
	bool updateControllers = true;
#endif
};
