#include "StateTest.h"

#include "vulkan/vulkan.h"

#include <numbers>

#include "Engine/Engine.h"
#include "Engine/Physics.h"
#include "Engine/Input.h"

#include "Engine/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/Framebuffer.h"

#include "Graphics/Screenspace.h"
#include "Graphics/Image.h"
#include "Graphics/Material.h"

#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Pipeline.h"
#include "Graphics/MaterialManager.h"

#include <glm/ext.hpp>

#include "Graphics/Conversions.h"
#include "Engine/Transform.h"

#include "Engine/Camera/FlyCamera.h"

#if defined(ENABLE_XR)
#	include "Graphics/VRGraphics.h"
#endif

#include <chrono>
#include <thread>
#include "Engine/Job.h"

struct MeshTestHolder {
	std::string mName;
	std::string mFilePath;
	std::string mTexturePath;
};
// clang-format off
const MeshTestHolder sceneMeshs[] = {
    {"Nothing", "", ""},
    {"Sponza", "/Assets/Cauldron-Media/Sponza/glTF/Sponza.gltf", "/Assets/Cauldron-Media/Sponza/glTF/"},
    {"Sponza-New", "/Assets/Cauldron-Media/Sponza-New/scene.gltf", "/Assets/Cauldron-Media/Sponza-New/"},
    {"BistroInterior", "/Assets/Cauldron-Media/BistroInterior/scene.gltf", "/Assets/Cauldron-Media/BistroInterior/"},
    {"HybridReflections", "/Assets/Cauldron-Media/HybridReflections/scene.gltf", "/Assets/Cauldron-Media/HybridReflections/"},
    {"SciFiHelmet", "/Assets/Cauldron-Media/SciFiHelmet/glTF/SciFiHelmet.gltf", "/Assets/Cauldron-Media/SciFiHelmet/glTF/"},
    {"MetalRoughSpheres", "/Assets/Cauldron-Media/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf", "/Assets/Cauldron-Media/MetalRoughSpheres/glTF/"},
    {"Brutalism", "/Assets/Cauldron-Media/Brutalism/BrutalistHall.gltf", "/Assets/Cauldron-Media/Brutalism/"},
    {"AbandonedWarehouse", "/Assets/Cauldron-Media/AbandonedWarehouse/AbandonedWarehouse.gltf", "/Assets/Cauldron-Media/AbandonedWarehouse/"},
    {"DamagedHelmet", "/Assets/Cauldron-Media/DamagedHelmet/glTF/DamagedHelmet.gltf", "/Assets/Cauldron-Media/DamagedHelmet/glTF/"},
};
const int numMesh = sizeof(sceneMeshs) / sizeof(MeshTestHolder);
// clang-format on

void StateTest::Initalize() {
	mSelectMeshRenderPass = new RenderPass();
	mSelectMeshFramebuffer = new Framebuffer();
	mSelectMeshPipeline = new Pipeline();
	mSelectMeshDepthImage = new Image();

	mMainRenderPass = new RenderPass();
	mSceneDataBuffer = new Buffer();
	mFbColorImage = new Image();
	mFbDepthImage = new Image();
	mMainFramebuffer = new Framebuffer();
	mMeshPipeline = new Pipeline();
	mScreenspaceBlit = new Screenspace();

	mSceneMesh = new Mesh();
	mControllerMesh = new Mesh();
	mWorldReferenceMesh = new Mesh();
	mPhysicsObjectMesh = new Mesh();

	mSceneModel = new Model();
	mControllerModel[0] = new Model();
	mControllerModel[1] = new Model();
	mWorldReferenceModel = new Model();
	for(int i = 0; i < cNumChainObjects; i++) {
		mChainModels[i] = new Model();
	}
#if defined(ENABLE_XR)
	mVrBlitPass = new Screenspace();
#endif
}

void StateTest::StartUp() {
	//move to engine/graphics?
#if defined(ENABLE_XR)
	mMainRenderPass->SetMultiViewSupport(true);
	mSelectMeshRenderPass->SetMultiViewSupport(true);
#endif
	mMainRenderPass->AddColorAttachment(Graphics::GetDeafultColorFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR);
	mMainRenderPass->AddDepthAttachment(Graphics::GetDeafultDepthFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR);
	mMainRenderPass->Create("Main Render RP");

	mSelectMeshRenderPass->AddColorAttachment(Graphics::GetDeafultColorFormat(), VK_ATTACHMENT_LOAD_OP_LOAD);
	mSelectMeshRenderPass->AddDepthAttachment(Graphics::GetDeafultDepthFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR);
	mSelectMeshRenderPass->Create("Select Model Render Pass");

	{
		std::vector<VkClearValue> mainPassClear(2);
		mainPassClear[0].color.float32[0] = 0.0f;
		mainPassClear[0].color.float32[1] = 0.0f;
		mainPassClear[0].color.float32[2] = 0.0f;
		mainPassClear[0].color.float32[3] = 1.0f;
		mainPassClear[1].depthStencil.depth = 1.0f;
		mainPassClear[1].depthStencil.stencil = 0;
		mMainRenderPass->SetClearColors(mainPassClear);
	}
	{
		std::vector<VkClearValue> selectModelClear(2);
		selectModelClear[1].depthStencil.depth = 1.0f;
		selectModelClear[1].depthStencil.stencil = 0;
		mSelectMeshRenderPass->SetClearColors(selectModelClear);
	}

	//FRAMEBUFFER TEST

	auto CreateSizeDependentRenderObjects = [&]() {
		mMainFramebuffer->Destroy();
		mSelectMeshFramebuffer->Destroy();

		mFbDepthImage->Destroy();
		mFbColorImage->Destroy();
		mFbColorImage->SetArrayLayers(gGraphics->GetNumActiveViews());
		mFbDepthImage->SetArrayLayers(gGraphics->GetNumActiveViews());
		mFbColorImage->CreateVkImage(Graphics::GetDeafultColorFormat(), gGraphics->GetDesiredSize(), true, "Main FB Image");
		mFbDepthImage->CreateVkImage(Graphics::GetDeafultDepthFormat(), gGraphics->GetDesiredSize(), true, "Main FB Depth Image");

		mSelectMeshDepthImage->Destroy();
		mSelectMeshDepthImage->SetArrayLayers(gGraphics->GetNumActiveViews());
		mSelectMeshDepthImage->CreateVkImage(Graphics::GetDeafultDepthFormat(), gGraphics->GetDesiredSize(), true, "Selected Mesh FB Depth Image");

		//convert to correct layout
		{
			auto buffer = gGraphics->AllocateGraphicsCommandBuffer();
			mFbColorImage->SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			mFbDepthImage->SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			mSelectMeshDepthImage->SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			gGraphics->EndGraphicsCommandBuffer(buffer);
		}

		//FB write to these images, following the layout from mMainRenderPass
		mMainFramebuffer->AddImage(mFbColorImage);
		mMainFramebuffer->AddImage(mFbDepthImage);
		mMainFramebuffer->Create(*mMainRenderPass, "Main Render FB");
		mSelectMeshFramebuffer->AddImage(mFbColorImage);
		mSelectMeshFramebuffer->AddImage(mSelectMeshDepthImage);
		mSelectMeshFramebuffer->Create(*mSelectMeshRenderPass, "Select Mesh FB");
	};
	CreateSizeDependentRenderObjects();
	gGraphics->mResizeMessage.AddCallback(CreateSizeDependentRenderObjects);

	//SCREEN SPACE TEST
	//used to copy mFbColorImage to backbuffer

	mScreenspaceMaterialBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	//mScreenspaceMaterialBase.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	mScreenspaceMaterialBase.Create();
	mScreenspaceBlit->AddMaterialBase(&mScreenspaceMaterialBase);
	{
		std::vector<VkClearValue> ssClear(1);
		ssClear[0].color.float32[0] = 0.0f;
		ssClear[0].color.float32[1] = 0.0f;
		ssClear[0].color.float32[2] = 0.0f;
		ssClear[0].color.float32[3] = 1.0f;
		mScreenspaceBlit->SetClearColors(ssClear);
	}
#if defined(ENABLE_XR)
	//xr needs the array version
	mScreenspaceBlit->Create("/Shaders/Screenspace/ImageSingleArray.frag.spv", "ScreenSpace ImageCopy");
	//mirrors the left eye to the PC display

	mVrBlitPass->mAttachmentFormat = gGraphics->GetSwapchainFormat();
	mVrBlitPass->AddMaterialBase(&mScreenspaceMaterialBase);
	mVrBlitPass->Create("/Shaders/Screenspace/ImageTwoArray.frag.spv", "ScreenSpace Mirror ImageCopy");
#else
	//windows doesnt do multiview so just needs the non array version
	mScreenspaceBlit->Create("/Shaders/Screenspace/ImageSingle.frag.spv", "ScreenSpace ImageCopy");
#endif
	auto SetupSSImages = [&]() {
#if defined(ENABLE_XR)
		mVrBlitPass->GetMaterial(0).SetImages(*mFbColorImage, 0, 0);
#endif
		mScreenspaceBlit->GetMaterial(0).SetImages(*mFbColorImage, 0, 0);
	};
	SetupSSImages();
	gGraphics->mResizeMessage.AddCallback(SetupSSImages);

	mControllerMesh->LoadMesh(std::string(WORK_DIR_REL) + "/Assets/handModel2.fbx");

	mWorldReferenceMesh->LoadMesh(std::string(WORK_DIR_REL) + "/Assets/5m reference.fbx");

	mPhysicsObjectMesh->LoadMesh(std::string(WORK_DIR_REL) + "/Assets/box.gltf");

	//inital load
	ChangeMesh(mSceneSelectedMeshIndex);

	mMeshPipeline->AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	mMeshPipeline->AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	mSelectMeshPipeline->AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshSelect.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	mSelectMeshPipeline->AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	{
		mMeshPipeline->vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		mMeshPipeline->vertexBinding.stride = sizeof(MeshVert);
		mMeshPipeline->vertexAttribute = std::vector<VkVertexInputAttributeDescription>(4);
		mMeshPipeline->vertexAttribute[0].location = 0;
		mMeshPipeline->vertexAttribute[0].format = VK_FORMAT_R32G32B32_SFLOAT; //3
		mMeshPipeline->vertexAttribute[0].offset = offsetof(MeshVert, mPos);
		mMeshPipeline->vertexAttribute[1].location = 1;
		mMeshPipeline->vertexAttribute[1].format = VK_FORMAT_R32G32B32_SFLOAT; //6
		mMeshPipeline->vertexAttribute[1].offset = offsetof(MeshVert, mNorm);
		mMeshPipeline->vertexAttribute[2].location = 2;
		mMeshPipeline->vertexAttribute[2].format = VK_FORMAT_R32G32B32A32_SFLOAT; //10
		mMeshPipeline->vertexAttribute[2].offset = offsetof(MeshVert, mColors[0]);
		mMeshPipeline->vertexAttribute[3].location = 3;
		mMeshPipeline->vertexAttribute[3].format = VK_FORMAT_R32G32_SFLOAT; //12
		mMeshPipeline->vertexAttribute[3].offset = offsetof(MeshVert, mUVs[0]);
		mSelectMeshPipeline->vertexBinding = mMeshPipeline->vertexBinding;
		mSelectMeshPipeline->vertexAttribute = mMeshPipeline->vertexAttribute;
	}

	VkPushConstantRange meshPCRange {};
	meshPCRange.size = sizeof(MeshPCTest);
	meshPCRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	mMeshPipeline->AddPushConstant(meshPCRange);
	mSelectMeshPipeline->AddPushConstant(meshPCRange);

	mSceneDataBuffer->Create(BufferType::UNIFORM, sizeof(SceneData), "Mesh Scene Uniform");

	mMeshSceneMaterialBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
	mMeshSceneMaterialBase.Create();
	mMeshPipeline->AddMaterialBase(&mMeshSceneMaterialBase);
	mMeshPipeline->AddBindlessTexture();
	mMeshPipeline->Create(mMainRenderPass->GetRenderPass(), "Mesh");
	mSelectMeshPipeline->AddMaterialBase(&mMeshSceneMaterialBase);
	mSelectMeshPipeline->AddBindlessTexture();
	mSelectMeshPipeline->Create(mSelectMeshRenderPass->GetRenderPass(), "Select Mesh");

	mMeshSceneDataMaterial = mMeshSceneMaterialBase.MakeMaterials()[0];
	mMeshSceneDataMaterial.SetBuffers(*mSceneDataBuffer, 0, 0);

	mControllerModel[0]->SetMesh(mControllerMesh);
	mControllerModel[1]->SetMesh(mControllerMesh);
	mWorldReferenceModel->SetMesh(mWorldReferenceMesh);
	{
		//
		mWorldBasePhysicsTest.AttachTransform(&mWorldReferenceModel->mLocation);
		mWorldBasePhysicsTest.AttachOther(mWorldReferenceModel);
		const float scale = 1.0f;
		std::vector<SimpleTransform> transforms = {
			SimpleTransform(glm::vec3(5, 0, 0), scale), SimpleTransform(glm::vec3(0, 5, 0), scale), SimpleTransform(glm::vec3(0, 0, 5), scale)};
		gPhysics->AddingObjectsTestCompoundBoxs(&mWorldBasePhysicsTest, transforms);
	}
	for(int i = 0; i < cNumChainObjects; i++) {
		mChainModels[i]->SetMesh(mPhysicsObjectMesh);
		mChainModels[i]->mOverrideColor = true;
		mChainModels[i]->mColorOverride = glm::vec4(glm::vec3(1.0f - (i / (float)cNumChainObjects)), 1.0f);

		mChainPhysicsObjects[i].AttachTransform(&mChainModels[i]->mLocation);
		mChainPhysicsObjects[i].AttachOther(mChainModels[i]);
		gPhysics->AddingObjectsTestBox(&mChainPhysicsObjects[i]);
	}
	mChainPhysicsObjects[cNumChainObjects - 1].SetMass(0.0f);
	mChainPhysicsObjects[cNumChainObjects - 1].SetKinematic(true);
	//mChainPhysicsObjects[0].SetKinematic(true);
	mChainModels[cNumChainObjects - 1]->mColorOverride = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	SetupPhysicsObjects();

	mSceneModel->SetMesh(mSceneMesh);

	gEngine->SetMainCamera(&mFlyCamera);
#if defined(ENABLE_XR)
	mVrCharacter.SetPosition(glm::vec3(8.0f, 0.2f, 0.0f));
	camera.mTransform.SetParent(&mVrCharacter);
	mControllerModel[0]->mLocation.SetParent(&mVrCharacter);
	mControllerModel[0]->mLocation.SetScale(0);
	mControllerModel[1]->mLocation.SetParent(&mVrCharacter);
	mControllerModel[1]->mLocation.SetScale(0);
	{
		//SimpleTransform offset = SimpleTransform(glm::vec3(0, 0, -5), 10.0f);
		//mControllerPhysObj[0].AttachTransform(&mControllerModel[0]->mLocation);
		//gPhysics->AddingObjectsTestCompoundBoxs(&mControllerPhysObj[0], {offset});
		//mControllerPhysObj[0].SetMass(0.0f);
		//mControllerPhysObj[1].AttachTransform(&mControllerModel[1]->mLocation);
		//gPhysics->AddingObjectsTestCompoundBoxs(&mControllerPhysObj[1], {offset});
		//mControllerPhysObj[1].SetMass(0.0f);
	}
#else
	mControllerModel[0]->mLocation.SetScale(0);
	mControllerModel[1]->mLocation.SetScale(0);
#endif

	//Xr does not need to respond to resize messages
	//it's fov comes from XR land
#if !defined(ENABLE_XR)
	auto SetupCameraAspect = [&]() {
		const ImageSize size = gGraphics->GetDesiredSize();
		float aspect = size.mWidth / (float)size.mHeight;
		mFlyCamera.SetFov(mFlyCamera.GetFovDegrees(), aspect);
	};
	SetupCameraAspect();
	gGraphics->mResizeMessage.AddCallback(SetupCameraAspect);
#endif

	//lighting test
	{
		mMeshSceneData.mDirectionalLight.mDirection = glm::vec4(-0.2f, -1.0f, -0.3f, 0);
		mMeshSceneData.mDirectionalLight.mAmbient = glm::vec4(0.20f, 0.2f, 0.2f, 0);
		mMeshSceneData.mDirectionalLight.mDiffuse = glm::vec4(0.8f, 0.8f, 0.8f, 0);
		mMeshSceneData.mDirectionalLight.mSpecular = glm::vec4(0.5f, 0.5f, 0.5f, 0);
	}
}

void StateTest::ImGuiRender() {
	if(ImGui::Begin("Camera")) {
		glm::vec3 camPos = mFlyCamera.mTransform.GetLocalPosition();
		if(ImGui::DragFloat3("Pos", glm::value_ptr(camPos), 0.1f, -999, 999)) {
			mFlyCamera.mTransform.SetPosition(camPos);
		}
		glm::vec3 camRot = mFlyCamera.mTransform.GetLocalRotationEuler();
		if(ImGui::DragFloat3("Rot", glm::value_ptr(camRot), 0.1f, -999, 999)) {
			mFlyCamera.mTransform.SetRotation(camRot);
		}
		glm::vec2 mousePos = gInput->GetMousePos();
		ImGui::Text("MousePos (%f,%f)", mousePos.x, mousePos.y);
		glm::vec2 mouseDelta = gInput->GetMouseDelta();
		ImGui::Text("mouseDelta (%f,%f)", mouseDelta.x, mouseDelta.y);
		ImGui::Text("Is Up? %i", mFlyCamera.mTransform.IsUp());
		if(ImGui::Button("Look at 0")) {
			mFlyCamera.mTransform.SetLookAt(mFlyCamera.mTransform.GetLocalPosition(), glm::vec3(0), CONSTANTS::UP);
		}
	}
	ImGui::End();

	if(ImGui::Begin("Lighting")) {
		ImGui::DragFloat3("Direction", glm::value_ptr(mMeshSceneData.mDirectionalLight.mDirection), 0.01f, -1.0f, 1.0f);
		ImGui::ColorEdit3("Ambient", glm::value_ptr(mMeshSceneData.mDirectionalLight.mAmbient));
		ImGui::ColorEdit3("Diffuse", glm::value_ptr(mMeshSceneData.mDirectionalLight.mDiffuse));
		ImGui::ColorEdit3("Specular", glm::value_ptr(mMeshSceneData.mDirectionalLight.mSpecular));
	}
	ImGui::End();

	if(ImGui::Begin("Scene")) {
		//ImGui::Checkbox("UpdateRoot", &updateRoot);
#if defined(ENABLE_XR)
		ImGui::Checkbox("updateControllers", &updateControllers);
#endif
		if(ImGui::BeginCombo("Scene Mesh", sceneMeshs[mSceneSelectedMeshIndex].mName.c_str())) {

			for(int i = 0; i < numMesh; i++) {
				const bool is_selected = (mSceneSelectedMeshIndex == i);
				if(ImGui::Selectable(sceneMeshs[i].mName.c_str(), is_selected)) {
					ChangeMesh(i);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::BeginDisabled();
		bool loaded = mSceneMesh->HasLoaded();
		ImGui::Checkbox("Has Loaded", &loaded);
		ImGui::EndDisabled();
		if(ImGui::Button("Reset Physics Objects")) {
			SetupPhysicsObjects();
		}
	}
	ImGui::End();
}

void StateTest::Update() {
	{
		glm::mat4 proj;
		glm::mat4 view;
		glm::mat4 world;
		Camera& camera = *gEngine->GetMainCamera();

#if defined(ENABLE_XR)
		VRGraphics::Pose head;
		gVrGraphics->GetHeadPoseData(head);
		glm::mat4 headMatrix;
		{
			glm::mat4 translation;
			glm::mat4 rotation;

			translation = glm::translate(camera.mTransform.GetWorldMatrix(), head.mPos);
			rotation = glm::mat4_cast(head.mRot);
			headMatrix = translation * rotation;
		}
		VRGraphics::View info;
		for(int i = 0; i < 2; i++) {
			gVrGraphics->GetEyePoseData(i, info);

			glm::mat4 translation;
			glm::mat4 rotation;

			translation = glm::translate(glm::identity<glm::mat4>(), info.mPos);
			rotation = glm::mat4_cast(info.mRot);

			view = camera.mTransform.GetWorldMatrix() * (translation * rotation);
			view = glm::inverse(view);
			glm::mat4 frustum =
				glm::frustum(tan(info.mFov.x) / 10, tan(info.mFov.y) / 10, tan(info.mFov.z) / 10, tan(info.mFov.w) / 10, 0.1f, 1000.0f);

			glm::mat4 resultm {};
			{
				const auto& tanAngleLeft = tan(info.mFov.x);
				const auto& tanAngleRight = tan(info.mFov.y);
				const auto& tanAngleUp = tan(info.mFov.z);
				const auto& tanAngleDown = tan(info.mFov.w);

				const float tanAngleWidth = tanAngleRight - tanAngleLeft;
				const float tanAngleHeight = (tanAngleDown - tanAngleUp);
				const float offsetZ = 0;

				float nearZ = 0.1f;
				float farZ = 1000.0f;

				float* result = &resultm[0][0];
				// normal projection
				result[0] = 2 / tanAngleWidth;
				result[4] = 0;
				result[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
				result[12] = 0;

				result[1] = 0;
				result[5] = 2 / tanAngleHeight;
				result[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
				result[13] = 0;

				result[2] = 0;
				result[6] = 0;
				result[10] = -(farZ + offsetZ) / (farZ - nearZ);
				result[14] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

				result[3] = 0;
				result[7] = 0;
				result[11] = -1;
				result[15] = 0;
			}

			proj = resultm;
			proj[1][1] *= -1.0f;

			mMeshSceneData.mPV[i] = proj * view;
			mMeshSceneData.mViewPos[i] = view[3];
		}
#else
		mMeshSceneData.mPV[0] = camera.GetViewProjMatrix();
		mMeshSceneData.mViewPos[0] = glm::vec4(camera.mTransform.GetWorldPosition(), 1);
#endif

		mSceneDataBuffer->UpdateData(0, VK_WHOLE_SIZE, &mMeshSceneData);
	}

	{
		const float time = gEngine->GetTimeSinceStart() * 50;
		//if(updateRoot) {
		//	mRootTransform.SetRotation(glm::vec3(0, -time, 0));
		//
		//	modelTest2.mLocation.SetRotation(glm::vec3(0, time, 0));
		//}
#if defined(ENABLE_XR)
		if(updateControllers) {
			for(int i = 0; i < VRGraphics::Side::COUNT; i++) {
				VRGraphics::ControllerInfo info;
				gVrGraphics->GetHandInfo((VRGraphics::Side)i, info);

				//rotate camera based on trackpad
				mVrCharacter.RotateAxis(-info.mTrackpad.x, CONSTANTS::UP);

				//move camera based on trigger
				glm::vec3 cameraMovement = glm::vec3(0);
				if(info.mTrigger > 0) {
					cameraMovement = glm::vec4(info.mLinearVelocity, 1) * -info.mTrigger;
					mVrCharacter.TranslateLocal(cameraMovement);
				}

				//move controller
				if(info.mActive) {
					mControllerModel[i]->mLocation.SetPosition(info.mPose.mPos + (-cameraMovement));
					mControllerModel[i]->mLocation.SetRotation(info.mPose.mRot);
					if(i == 0) {
						mControllerModel[i]->mLocation.SetScale(glm::vec3(-1.0f, 1.0f, 1.0f));
					} else {
						mControllerModel[i]->mLocation.SetScale(1.0f);
					}
					//mControllerPhysObj[i].UpdateToPhysics();
				} else {
					mControllerModel[i]->mLocation.SetScale(0.0f);
				}
			}
		}
#endif
	}

	static bool scenePhysicsTest = false;
	if(mSceneMesh->HasLoaded() && mSceneMesh->GetNumMesh() != 0) {
		if(scenePhysicsTest == false) {
			scenePhysicsTest = true;
			if(mScenePhysicsObject.GetTransform() == nullptr) {
				mScenePhysicsObject.AttachTransform(&mSceneModel->mLocation);
				mScenePhysicsObject.AttachOther(mSceneModel);
			}

			//mSceneModel->SetMesh(mPhysicsObjectMesh);
			//gPhysics->AddingObjectsTestMesh(&mScenePhysicsObject, mPhysicsObjectMesh);
			gPhysics->AddingObjectsTestMesh(&mScenePhysicsObject, mSceneMesh);
		}
	} else {
		if(mScenePhysicsObject.GetRigidBody() != nullptr) {
			gPhysics->RemovePhysicsObject(&mScenePhysicsObject);
		}
		scenePhysicsTest = false;
	}

	if(gInput->WasKeyPressed(GLFW_KEY_SPACE) || gInput->WasMouseButtonPressed(GLFW_MOUSE_BUTTON_1)) {
		mPhyBalls.push_back(new PhyBall());
		PhyBall& pyobj = *mPhyBalls.back();
		pyobj.model = new Model();
		pyobj.model->SetMesh(mPhysicsObjectMesh);
		pyobj.model->mLocation.CopyTransform(mFlyCamera.mTransform, Transform::POSITION);
		pyobj.model->mLocation.SetScale(0.2f);
		pyobj.pyObj.AttachTransform(&pyobj.model->mLocation);
		pyobj.pyObj.AttachOther(pyobj.model);
		gPhysics->AddingObjectsTestBox(&pyobj.pyObj);
		const float speed = 5.0f;
		if(gInput->WasMouseButtonPressed(GLFW_MOUSE_BUTTON_1)) {
			int width, height;
			gEngine->GetWindow()->GetSize(&width, &height);
			const glm::vec3 dir = mFlyCamera.GetWorldDirFromScreen(gInput->GetMousePos(), glm::vec2(width, height));
			pyobj.pyObj.SetVelocity(dir * speed);
		} else {
			pyobj.pyObj.SetVelocity(mFlyCamera.mTransform.GetForward() * -speed);
		}
		pyobj.model->mOverrideColor = true;
		pyobj.model->mColorOverride = glm::vec4(1.0f, 0, 0, 1);
	}

	{
		PhysicsObject* obj = nullptr;
#if defined(ENABLE_XR)
		VRGraphics::ControllerInfo info;
		for(int i = 0; i < VRGraphics::Side::COUNT; i++) {
			gVrGraphics->GetHandInfo((VRGraphics::Side)i, info);
			if(info.mActive) {
				obj =
					gPhysics->Raycast(mControllerModel[i]->mLocation.GetWorldPosition(), -mControllerModel[i]->mLocation.GetWorldForward(), 1000.0f);
				mControllerModel[(i + 1) % 2]->mLocation.SetWorldPosition(mControllerModel[i]->mLocation.GetWorldPosition() +
																		  -mControllerModel[i]->mLocation.GetWorldForward());
				//mControllerModel[(i + 1) % 2]->mLocation.SetWorldRotation(-mControllerModel[i]->mLocation.GetWorldForward());
				mControllerModel[(i + 1) % 2]->mLocation.SetScale(0.5f);
				break;
			}
		}
#else
		int width, height;
		gEngine->GetWindow()->GetSize(&width, &height);

		const glm::vec3 viewDir = mFlyCamera.GetWorldDirFromScreen(gInput->GetMousePos(), glm::vec2(width, height));

		obj = gPhysics->Raycast(mFlyCamera.mTransform.GetWorldPosition(), viewDir, 1000.0f);
#endif
		if(obj) {
			//mSelectedModel = (Model*)obj->GetOther();
		} else {
			mSelectedModel = nullptr;
		}
	}
}

void StateTest::Render() {
	VkCommandBuffer buffer = gGraphics->GetCurrentGraphicsCommandBuffer();

	//mesh render test
	mMainRenderPass->Begin(buffer, *mMainFramebuffer);
	mMeshPipeline->Begin(buffer);
	//bind camera data
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mMeshPipeline->GetLayout(), 0, 1, mMeshSceneDataMaterial.GetSet(), 0, nullptr);
	if(gGraphics->GetMaterialManager()->IsInLargeArrayMode()) {
		vkCmdBindDescriptorSets(buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								mMeshPipeline->GetLayout(),
								1,
								1,
								gGraphics->GetMaterialManager()->GetMainTextureSet(),
								0,
								nullptr);
	}

	//Hand 1
	mControllerModel[0]->Render(buffer, mMeshPipeline->GetLayout());
	//Hand 2
	mControllerModel[1]->Render(buffer, mMeshPipeline->GetLayout());
	//world base reference
	mWorldReferenceModel->Render(buffer, mMeshPipeline->GetLayout());
	for(int i = 0; i < cNumChainObjects; i++) {
		mChainModels[i]->Render(buffer, mMeshPipeline->GetLayout());
	}
	for(int i = 0; i < mPhyBalls.size(); i++) {
		mPhyBalls[i]->model->Render(buffer, mMeshPipeline->GetLayout());
	}
	mSceneModel->Render(buffer, mMeshPipeline->GetLayout());
	mMeshPipeline->End(buffer);
	mMainRenderPass->End(buffer);

	if(mSelectedModel != 0) {
		mSelectMeshRenderPass->Begin(buffer, *mSelectMeshFramebuffer);
		mSelectMeshPipeline->Begin(buffer);

		vkCmdBindDescriptorSets(
			buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSelectMeshPipeline->GetLayout(), 0, 1, mMeshSceneDataMaterial.GetSet(), 0, nullptr);
		if(gGraphics->GetMaterialManager()->IsInLargeArrayMode()) {
			vkCmdBindDescriptorSets(buffer,
									VK_PIPELINE_BIND_POINT_GRAPHICS,
									mSelectMeshPipeline->GetLayout(),
									1,
									1,
									gGraphics->GetMaterialManager()->GetMainTextureSet(),
									0,
									nullptr);
		}

		//glm::vec2 viewportMin = VulkanToGlm(mSelectMeshFramebuffer->GetSize()) * -0.2f;
		//glm::vec2 viewportMax = VulkanToGlm(mSelectMeshFramebuffer->GetSize()) * 1.4f;
		//VkViewport viewport = {viewportMin.x, viewportMin.y, viewportMax.x, viewportMax.y, 0.0f, 1.0f};
		//vkCmdSetViewport(buffer, 0, 1, &viewport);

		mSelectedModel->Render(buffer, mSelectMeshPipeline->GetLayout());

		mSelectMeshPipeline->End(buffer);
		mSelectMeshRenderPass->End(buffer);
	}

	mFbColorImage->SetImageLayout(buffer,
								  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
								  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	Screenspace::PushConstant pc;
	pc.mEyeIndex = 0;
	vkCmdPushConstants(buffer, mScreenspaceBlit->GetPipeline().GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Screenspace::PushConstant), &pc);
#if defined(ENABLE_XR)
	//copy mFbColorImage to computer view (using two eye version)
	mVrBlitPass->Render(buffer, gGraphics->GetCurrentFrameBuffer());
	//copy to headset views
	for(int i = 0; i < gGraphics->GetNumActiveViews(); i++) {
		pc.mEyeIndex = i;
		vkCmdPushConstants(
			buffer, mScreenspaceBlit->GetPipeline().GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Screenspace::PushConstant), &pc);
		//copies mFbColorImage to framebuffer
		mScreenspaceBlit->Render(buffer, gGraphics->GetCurrentXrFrameBuffer(i));
	}
#else
	mScreenspaceBlit->Render(buffer, gGraphics->GetCurrentFrameBuffer());
#endif

	mFbColorImage->SetImageLayout(buffer,
								  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void StateTest::Finish() {
	mRootTransform.Clear();

	mControllerModel[0]->Destroy();
	delete mControllerModel[0];
	mControllerModel[1]->Destroy();
	delete mControllerModel[1];
	mWorldReferenceModel->Destroy();
	delete mWorldReferenceModel;
	for(int i = 0; i < cNumChainObjects; i++) {
		mChainModels[i]->Destroy();
		delete mChainModels[i];
	}
	for(int i = 0; i < mPhyBalls.size(); i++) {
		mPhyBalls[i]->model->Destroy();
		mPhyBalls[i]->model = nullptr;
	}
	mPhyBalls.clear();

	mMeshPipeline->Destroy();
	delete mMeshPipeline;

	mPhysicsObjectMesh->Destroy();
	delete mPhysicsObjectMesh;
	mWorldReferenceMesh->Destroy();
	delete mWorldReferenceMesh;
	mControllerMesh->Destroy();
	delete mControllerMesh;
	mSceneMesh->Destroy();
	delete mSceneMesh;
	mSceneDataBuffer->Destroy();
	delete mSceneDataBuffer;

	mSelectMeshDepthImage->Destroy();
	delete mSelectMeshDepthImage;
	mSelectMeshPipeline->Destroy();
	delete mSelectMeshPipeline;
	mSelectMeshFramebuffer->Destroy();
	delete mSelectMeshFramebuffer;
	mSelectMeshRenderPass->Destroy();
	delete mSelectMeshRenderPass;

	mMainRenderPass->Destroy();
	delete mMainRenderPass;
	mMainFramebuffer->Destroy();
	delete mMainFramebuffer;
	mFbDepthImage->Destroy();
	delete mFbDepthImage;
	mFbColorImage->Destroy();
	delete mFbColorImage;

#if defined(ENABLE_XR)
	mVrCharacter.Clear(true);
	mVrBlitPass->Destroy();
	delete mVrBlitPass;
#endif
	mScreenspaceBlit->Destroy();
	delete mScreenspaceBlit;

	mMeshSceneMaterialBase.Destroy();
	mScreenspaceMaterialBase.Destroy();
}

void StateTest::ChangeMesh(int aIndex) {
	mSceneSelectedMeshIndex = aIndex;
	mSceneMesh->Destroy();
	mSceneMesh->LoadMesh(std::string(WORK_DIR_REL) + sceneMeshs[mSceneSelectedMeshIndex].mFilePath,
						 std::string(WORK_DIR_REL) + sceneMeshs[mSceneSelectedMeshIndex].mTexturePath);
}

void StateTest::SetupPhysicsObjects() {
	float offset = 10.0f;
	for(int i = 0; i < cNumChainObjects; i++) {
		if(mChainPhysicsLinks[i]) {
			gPhysics->RemoveContraintTemp(mChainPhysicsLinks[i]);
			mChainPhysicsLinks[i] = nullptr;
		}
		mChainPhysicsObjects[i].RemoveAttachmentsTemp();
	}
	for(int i = 0; i < cNumChainObjects; i++) {
		float scale = 0.5f + rand() % 1000 / 2000.0f;
		if(i == 0 || i == cNumChainObjects - 1) {
			scale = 0.1f;
		}
		offset += ((rand() % 1000 / 1000.0f) * 1.0f) + scale;
		mChainModels[i]->mLocation.SetPosition(glm::vec3((rand() % 1000 / 1000.0f) * 1.0f, offset, (rand() % 1000 / 1000.0f) * 1.0f));
		mChainModels[i]->mLocation.SetScale(scale);
		if(i != 0) {
			mChainPhysicsLinks[i] = gPhysics->JoinTwoObject(&mChainPhysicsObjects[i - 1], &mChainPhysicsObjects[i]);
		}
		mChainPhysicsObjects[i].ResetPhysics();
	}
}
