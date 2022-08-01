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

	Framebuffer* mSelectMeshFramebuffer = nullptr;
	RenderPass* mSelectMeshRenderPass = nullptr;
	Pipeline* mSelectMeshPipeline = nullptr;
	Image* mSelectMeshDepthImage = nullptr;
	Model* mSelectedModel = nullptr;

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
    PhysicsObject modelScenePhysicsObj;

	Model* modelTest1;
	Model* modelTest2;
	Model* mControllerTest[2];
	PhysicsObject mControllerPhysObj[2];
	Model* worldBase;
	PhysicsObject mWorldBasePhysicsTest;
	static const int numPhysicsObjects = 100;
	Model* physicsModels[numPhysicsObjects];
	PhysicsObject physicsObjects[numPhysicsObjects];
	class btTypedConstraint* mPhysicsLinks[numPhysicsObjects] = {nullptr};

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

	struct PhyBall {
		PhysicsObject pyObj;
		Model* model;
	};
	std::vector<PhyBall*> mPhyBalls;

#if defined(ENABLE_XR)
	Transform mVrCharacter;
	Screenspace* vrMirrorPass;
	bool updateControllers = true;
#endif
};
