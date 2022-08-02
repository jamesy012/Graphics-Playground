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

	RenderPass* mMainRenderPass;

    MaterialBase mMeshSceneMaterialBase;
	Buffer* mSceneDataBuffer;
	SceneData mMeshSceneData;
	Material mMeshSceneDataMaterial;

	Framebuffer* mSelectMeshFramebuffer = nullptr;
	RenderPass* mSelectMeshRenderPass = nullptr;
	Pipeline* mSelectMeshPipeline = nullptr;
	Image* mSelectMeshDepthImage = nullptr;
	Model* mSelectedModel = nullptr;

	Image* mFbColorImage;
	Image* mFbDepthImage;
	Framebuffer* mMainFramebuffer;
	Pipeline* mMeshPipeline;

    //big model that has a physics collision
	Mesh* mSceneMesh;
	Model* mSceneModel;
    PhysicsObject mScenePhysicsObject;
	int mSceneSelectedMeshIndex = 0;

    //xr controllers
	Mesh* mControllerMesh;
	Model* mControllerModel[2];
	PhysicsObject mControllerPhysObj[2];

    //XYZ marker in the world
	Mesh* mWorldReferenceMesh;
	Model* mWorldReferenceModel;
	PhysicsObject mWorldBasePhysicsTest;

    //common mesh for physics objects
	Mesh* mPhysicsObjectMesh;
	static const int cNumChainObjects = 100;
	Model* mChainModels[cNumChainObjects];
	PhysicsObject mChainPhysicsObjects[cNumChainObjects];
	class btTypedConstraint* mChainPhysicsLinks[cNumChainObjects] = {nullptr};

    //for coping mMainFramebuffer to the back buffer
	MaterialBase mScreenspaceMaterialBase;
	Screenspace* mScreenspaceBlit;

	Transform mRootTransform;
	FlyCamera mFlyCamera;
	struct PhyBall {
		PhysicsObject pyObj;
		Model* model;
	};
	std::vector<PhyBall*> mPhyBalls;

#if defined(ENABLE_XR)
	Transform mVrCharacter;
	Screenspace* mVrBlitPass;
	bool updateControllers = true;
#endif
};
