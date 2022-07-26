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
	mainRenderPass = new RenderPass();
	sceneDataBuffer = new Buffer();
	fbImage = new Image();
	fbDepthImage = new Image();
	fb = new Framebuffer();
	meshPipeline = new Pipeline();
	ssTest = new Screenspace();

	meshSceneTest = new Mesh();
	meshTest = new Mesh();
	handMesh = new Mesh();
	referenceMesh = new Mesh();
	physicsMesh = new Mesh();

	modelSceneTest = new Model();
	modelTest1 = new Model();
	modelTest2 = new Model();
	controllerTest1 = new Model();
	controllerTest2 = new Model();
	worldBase = new Model();
	for(int i = 0; i < numPhysicsObjects; i++) {
		physicsModels[i] = new Model();
	}
#if defined(ENABLE_XR)
	vrMirrorPass = new Screenspace();
#endif
}
void StateTest::StartUp() {
	//move to engine/graphics?
#if defined(ENABLE_XR)
	mainRenderPass->SetMultiViewSupport(true);
#endif
	mainRenderPass->AddColorAttachment(Graphics::GetDeafultColorFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR);
	mainRenderPass->AddDepthAttachment(Graphics::GetDeafultDepthFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR);
	mainRenderPass->Create("Main Render RP");

	{
		std::vector<VkClearValue> mainPassClear(2);
		mainPassClear[0].color.float32[0] = 0.0f;
		mainPassClear[0].color.float32[1] = 0.0f;
		mainPassClear[0].color.float32[2] = 0.0f;
		mainPassClear[0].color.float32[3] = 1.0f;
		mainPassClear[1].depthStencil.depth = 1.0f;
		mainPassClear[1].depthStencil.stencil = 0;
		mainRenderPass->SetClearColors(mainPassClear);
	}

	//FRAMEBUFFER TEST

	auto CreateSizeDependentRenderObjects = [&]() {
		fb->Destroy();
		fbDepthImage->Destroy();
		fbImage->Destroy();
		fbImage->SetArrayLayers(gGraphics->GetNumActiveViews());
		fbDepthImage->SetArrayLayers(gGraphics->GetNumActiveViews());
		fbImage->CreateVkImage(Graphics::GetDeafultColorFormat(), gGraphics->GetDesiredSize(), true, "Main FB Image");
		fbDepthImage->CreateVkImage(Graphics::GetDeafultDepthFormat(), gGraphics->GetDesiredSize(), true, "Main FB Depth Image");

		//convert to correct layout
		{
			auto buffer = gGraphics->AllocateGraphicsCommandBuffer();
			fbImage->SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			fbDepthImage->SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			gGraphics->EndGraphicsCommandBuffer(buffer);
		}

		//FB write to these images, following the layout from mainRenderPass
		fb->AddImage(fbImage);
		fb->AddImage(fbDepthImage);
		fb->Create(*mainRenderPass, "Main Render FB");
	};
	CreateSizeDependentRenderObjects();
	gGraphics->mResizeMessage.push_back(CreateSizeDependentRenderObjects);

	//SCREEN SPACE TEST
	//used to copy fbImage to backbuffer

	ssTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	//ssTestBase.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	ssTestBase.Create();
	ssTest->AddMaterialBase(&ssTestBase);
	{
		std::vector<VkClearValue> ssClear(1);
		ssClear[0].color.float32[0] = 0.0f;
		ssClear[0].color.float32[1] = 0.0f;
		ssClear[0].color.float32[2] = 0.0f;
		ssClear[0].color.float32[3] = 1.0f;
		ssTest->SetClearColors(ssClear);
	}
#if defined(ENABLE_XR)
	//xr needs the array version
	ssTest->Create("/Shaders/Screenspace/ImageSingleArray.frag.spv", "ScreenSpace ImageCopy");
	//mirrors the left eye to the PC display

	vrMirrorPass->mAttachmentFormat = gGraphics->GetSwapchainFormat();
	vrMirrorPass->AddMaterialBase(&ssTestBase);
	vrMirrorPass->Create("/Shaders/Screenspace/ImageTwoArray.frag.spv", "ScreenSpace Mirror ImageCopy");
#else
	//windows doesnt do multiview so just needs the non array version
	ssTest->Create("/Shaders/Screenspace/ImageSingle.frag.spv", "ScreenSpace ImageCopy");
#endif
	auto SetupSSImages = [&]() {
#if defined(ENABLE_XR)
		vrMirrorPass->GetMaterial(0).SetImages(*fbImage, 0, 0);
#endif
		ssTest->GetMaterial(0).SetImages(*fbImage, 0, 0);
	};
	SetupSSImages();
	gGraphics->mResizeMessage.push_back(SetupSSImages);

	//Mesh Test
	meshTest->LoadMesh(std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/MapleTree_5.fbx",
					   std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/");

	handMesh->LoadMesh(std::string(WORK_DIR_REL) + "/Assets/handModel2.fbx");

	referenceMesh->LoadMesh(std::string(WORK_DIR_REL) + "/Assets/5m reference.fbx");

	physicsMesh->LoadMesh(std::string(WORK_DIR_REL) + "/Assets/box.gltf");

	//inital load
	ChangeMesh(selectedMesh);

	meshPipeline->AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	meshPipeline->AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	{
		meshPipeline->vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		meshPipeline->vertexBinding.stride = sizeof(MeshVert);
		meshPipeline->vertexAttribute = std::vector<VkVertexInputAttributeDescription>(4);
		meshPipeline->vertexAttribute[0].location = 0;
		meshPipeline->vertexAttribute[0].format = VK_FORMAT_R32G32B32_SFLOAT; //3
		meshPipeline->vertexAttribute[0].offset = offsetof(MeshVert, mPos);
		meshPipeline->vertexAttribute[1].location = 1;
		meshPipeline->vertexAttribute[1].format = VK_FORMAT_R32G32B32_SFLOAT; //6
		meshPipeline->vertexAttribute[1].offset = offsetof(MeshVert, mNorm);
		meshPipeline->vertexAttribute[2].location = 2;
		meshPipeline->vertexAttribute[2].format = VK_FORMAT_R32G32B32A32_SFLOAT; //10
		meshPipeline->vertexAttribute[2].offset = offsetof(MeshVert, mColors[0]);
		meshPipeline->vertexAttribute[3].location = 3;
		meshPipeline->vertexAttribute[3].format = VK_FORMAT_R32G32_SFLOAT; //12
		meshPipeline->vertexAttribute[3].offset = offsetof(MeshVert, mUVs[0]);
	}

	VkPushConstantRange meshPCRange {};
	meshPCRange.size = sizeof(MeshPCTest);
	meshPCRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	meshPipeline->AddPushConstant(meshPCRange);

	sceneDataBuffer->Create(BufferType::UNIFORM, sizeof(SceneData), "Mesh Scene Uniform");

	meshTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
	meshTestBase.Create();
	meshPipeline->AddMaterialBase(&meshTestBase);
	meshPipeline->AddBindlessTexture();
	meshPipeline->Create(mainRenderPass->GetRenderPass(), "Mesh");

	meshMaterial = meshTestBase.MakeMaterials()[0];
	meshMaterial.SetBuffers(*sceneDataBuffer, 0, 0);

	modelTest1->SetMesh(meshTest);
	modelTest2->SetMesh(meshTest);
	controllerTest1->SetMesh(handMesh);
	controllerTest2->SetMesh(handMesh);
	worldBase->SetMesh(referenceMesh);
	for(int i = 0; i < numPhysicsObjects; i++) {
		physicsModels[i]->SetMesh(physicsMesh);
		physicsObjects[i].AttachTransform(&physicsModels[i]->mLocation);
		gPhysics->AddingObjectsTestSphere(&physicsObjects[i]);
		if(i != 0) {
			gPhysics->JoinTwoObject(&physicsObjects[i - 1], &physicsObjects[i]);
		}
		physicsModels[i]->mOverrideColor = true;
		physicsModels[i]->mColorOverride = glm::vec4(glm::vec3(i / (float)numPhysicsObjects), 1.0f);
	}
	physicsObjects[numPhysicsObjects - 1].SetMass(0.01f);
	physicsModels[numPhysicsObjects - 1]->mColorOverride = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	SetupPhysicsObjects();

	mGroundTransform.SetPosition(glm::vec3(0, -50, 0));
	mGroundTransform.SetScale(glm::vec3(50));
	mGroundPlane.AttachTransform(&mGroundTransform);
	gPhysics->AddingObjectsTestGround(&mGroundPlane);

	modelSceneTest->SetMesh(meshSceneTest);

	modelTest1->mLocation.Set(glm::vec3(0, 0, 0), 1.0f, &mRootTransform);

	gEngine->SetMainCamera(&camera);
	camera.mTransform.SetPosition(glm::vec3(8.0f, 0.2f, 0.0f));
	controllerTest1->mLocation.SetParent(&camera.mTransform);
	controllerTest2->mLocation.SetParent(&camera.mTransform);
	controllerTest1->mLocation.SetScale(0);
	controllerTest2->mLocation.SetScale(0);
	//Xr does not need to respond to resize messages
	//it's fov comes from XR land
#if !defined(ENABLE_XR)
	auto SetupCameraAspect = [&]() {
		const ImageSize size = gGraphics->GetDesiredSize();
		float aspect = size.mWidth / (float)size.mHeight;
		camera.SetFov(camera.GetFovDegrees(), aspect);
	};
	SetupCameraAspect();
	gGraphics->mResizeMessage.push_back(SetupCameraAspect);
#endif

	//lighting test
	{
		sceneData.mDirectionalLight.mDirection = glm::vec4(-0.2f, -1.0f, -0.3f, 0);
		sceneData.mDirectionalLight.mAmbient = glm::vec4(0.20f, 0.2f, 0.2f, 0);
		sceneData.mDirectionalLight.mDiffuse = glm::vec4(0.8f, 0.8f, 0.8f, 0);
		sceneData.mDirectionalLight.mSpecular = glm::vec4(0.5f, 0.5f, 0.5f, 0);
	}
}

void StateTest::ImGuiRender() {
	if(ImGui::Begin("Camera")) {
		glm::vec3 camPos = camera.mTransform.GetLocalPosition();
		if(ImGui::DragFloat3("Pos", glm::value_ptr(camPos), 0.1f, -999, 999)) {
			camera.mTransform.SetPosition(camPos);
		}
		glm::vec3 camRot = camera.mTransform.GetLocalRotationEuler();
		if(ImGui::DragFloat3("Rot", glm::value_ptr(camRot), 0.1f, -999, 999)) {
			camera.mTransform.SetRotation(camRot);
		}
		glm::vec2 mousePos = gInput->GetMousePos();
		ImGui::Text("MousePos (%f,%f)", mousePos.x, mousePos.y);
		glm::vec2 mouseDelta = gInput->GetMouseDelta();
		ImGui::Text("mouseDelta (%f,%f)", mouseDelta.x, mouseDelta.y);
		ImGui::Text("Is Up? %i", camera.mTransform.IsUp());
		if(ImGui::Button("Look at 0")) {
			camera.mTransform.SetLookAt(camera.mTransform.GetLocalPosition(), glm::vec3(0), CONSTANTS::UP);
		}
	}
	ImGui::End();

	if(ImGui::Begin("Lighting")) {
		ImGui::DragFloat3("Direction", glm::value_ptr(sceneData.mDirectionalLight.mDirection), 0.01f, -1.0f, 1.0f);
		ImGui::ColorEdit3("Ambient", glm::value_ptr(sceneData.mDirectionalLight.mAmbient));
		ImGui::ColorEdit3("Diffuse", glm::value_ptr(sceneData.mDirectionalLight.mDiffuse));
		ImGui::ColorEdit3("Specular", glm::value_ptr(sceneData.mDirectionalLight.mSpecular));
	}
	ImGui::End();

	if(ImGui::Begin("Scene")) {
		//ImGui::Checkbox("UpdateRoot", &updateRoot);
#if defined(ENABLE_XR)
		ImGui::Checkbox("updateControllers", &updateControllers);
#endif
		if(ImGui::BeginCombo("Scene Mesh", sceneMeshs[selectedMesh].mName.c_str())) {

			for(int i = 0; i < numMesh; i++) {
				const bool is_selected = (selectedMesh == i);
				if(ImGui::Selectable(sceneMeshs[i].mName.c_str(), is_selected)) {
					ChangeMesh(i);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::BeginDisabled();
		bool loaded = meshSceneTest->HasLoaded();
		ImGui::Checkbox("Has Loaded", &loaded);
		ImGui::EndDisabled();
		ImGui::Checkbox("Render Trees?", &mRenderTrees);
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

			sceneData.mPV[i] = proj * view;
			sceneData.mViewPos[i] = view[3];
		}
#else
		sceneData.mPV[0] = camera.GetViewProjMatrix();
		sceneData.mViewPos[0] = glm::vec4(camera.mTransform.GetWorldPosition(), 1);
#endif

		sceneDataBuffer->UpdateData(0, VK_WHOLE_SIZE, &sceneData);
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
			Transform* controllerTransforms[2] = {&controllerTest1->mLocation, &controllerTest2->mLocation};
			for(int i = 0; i < VRGraphics::Side::COUNT; i++) {
				VRGraphics::ControllerInfo info;
				gVrGraphics->GetHandInfo((VRGraphics::Side)i, info);

				//rotate camera based on trackpad
				camera.mTransform.RotateAxis(-info.mTrackpad.x, CONSTANTS::UP);

				//move camera based on trigger
				glm::vec3 cameraMovement = glm::vec3(0);
				if(info.mTrigger > 0) {
					cameraMovement = glm::vec4(info.mLinearVelocity, 1) * -info.mTrigger;
					camera.mTransform.TranslateLocal(cameraMovement);
				}

				//move controller
				if(info.mActive) {
					controllerTransforms[i]->SetPosition(info.mPose.mPos + (-cameraMovement));
					controllerTransforms[i]->SetRotation(info.mPose.mRot);
					controllerTransforms[i]->SetScale(1.0f);

				} else {
					controllerTransforms[i]->SetScale(0.0f);
				}
			}
		}
#endif
	}

	if(referenceMesh->HasLoaded() && mWorldBasePhysicsTest.GetRigidBody() == nullptr) {
		mWorldBasePhysicsTest.AttachTransform(&worldBase->mLocation);
		gPhysics->AddingObjectsTestMesh(&mWorldBasePhysicsTest, referenceMesh);
	}
}

void StateTest::Render() {
	VkCommandBuffer buffer = gGraphics->GetCurrentGraphicsCommandBuffer();

	const Framebuffer& framebuffer = *fb;

	//mesh render test
	mainRenderPass->Begin(buffer, framebuffer);
	meshPipeline->Begin(buffer);
	//bind camera data
	vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->GetLayout(), 0, 1, meshMaterial.GetSet(), 0, nullptr);
	if(gGraphics->GetMaterialManager()->IsInLargeArrayMode()) {
		vkCmdBindDescriptorSets(buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								meshPipeline->GetLayout(),
								1,
								1,
								gGraphics->GetMaterialManager()->GetMainTextureSet(),
								0,
								nullptr);
	}
	if(mRenderTrees) {
		//tree 1
		modelTest1->Render(buffer, meshPipeline->GetLayout());
		//tree 2
		modelTest2->Render(buffer, meshPipeline->GetLayout());
	}
	//Hand 1
	controllerTest1->Render(buffer, meshPipeline->GetLayout());
	//Hand 2
	controllerTest2->Render(buffer, meshPipeline->GetLayout());
	//world base reference
	worldBase->Render(buffer, meshPipeline->GetLayout());
	for(int i = 0; i < numPhysicsObjects; i++) {
		physicsModels[i]->Render(buffer, meshPipeline->GetLayout());
	}
	modelSceneTest->Render(buffer, meshPipeline->GetLayout());
	meshPipeline->End(buffer);
	mainRenderPass->End(buffer);

	fbImage->SetImageLayout(buffer,
							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	Screenspace::PushConstant pc;
	pc.mEyeIndex = 0;
	vkCmdPushConstants(buffer, ssTest->GetPipeline().GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Screenspace::PushConstant), &pc);
#if defined(ENABLE_XR)
	//copy fbImage to computer view (using two eye version)
	vrMirrorPass->Render(buffer, gGraphics->GetCurrentFrameBuffer());
	//copy to headset views
	for(int i = 0; i < gGraphics->GetNumActiveViews(); i++) {
		pc.mEyeIndex = i;
		vkCmdPushConstants(buffer, ssTest->GetPipeline().GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Screenspace::PushConstant), &pc);
		//copies fbImage to framebuffer
		ssTest->Render(buffer, gGraphics->GetCurrentXrFrameBuffer(i));
	}
#else
	ssTest->Render(buffer, gGraphics->GetCurrentFrameBuffer());
#endif

	fbImage->SetImageLayout(buffer,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
							VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void StateTest::Finish() {
	mRootTransform.Clear();

	modelTest1->Destroy();
	delete modelTest1;
	modelTest2->Destroy();
	delete modelTest2;
	controllerTest1->Destroy();
	delete controllerTest1;
	controllerTest2->Destroy();
	delete controllerTest2;
	worldBase->Destroy();
	delete worldBase;
	for(int i = 0; i < numPhysicsObjects; i++) {
		physicsModels[i]->Destroy();
		delete physicsModels[i];
	}

	meshPipeline->Destroy();
	delete meshPipeline;

	physicsMesh->Destroy();
	delete physicsMesh;
	referenceMesh->Destroy();
	delete referenceMesh;
	handMesh->Destroy();
	delete handMesh;
	meshSceneTest->Destroy();
	delete meshSceneTest;
	meshTest->Destroy();
	delete meshTest;
	sceneDataBuffer->Destroy();
	delete sceneDataBuffer;

	mainRenderPass->Destroy();
	delete mainRenderPass;
	fb->Destroy();
	delete fb;
	fbDepthImage->Destroy();
	delete fbDepthImage;
	fbImage->Destroy();
	delete fbImage;

#if defined(ENABLE_XR)
	vrMirrorPass->Destroy();
	delete vrMirrorPass;
#endif
	ssTest->Destroy();
	delete ssTest;

	meshTestBase.Destroy();
	ssTestBase.Destroy();
}

void StateTest::ChangeMesh(int aIndex) {
	selectedMesh = aIndex;
	meshSceneTest->Destroy();
	meshSceneTest->LoadMesh(std::string(WORK_DIR_REL) + sceneMeshs[selectedMesh].mFilePath,
							std::string(WORK_DIR_REL) + sceneMeshs[selectedMesh].mTexturePath);
}

void StateTest::SetupPhysicsObjects() {
	float offset = 10.0f;
	for(int i = 0; i < numPhysicsObjects; i++) {
		float scale = 0.5f + rand() % 1000 / 2000.0f;
		offset += ((rand() % 1000 / 1000.0f) * 0.5f) + scale / 2;
		physicsModels[i]->mLocation.SetPosition(glm::vec3((rand() % 1000 / 1000.0f) * 1.0f, offset, (rand() % 1000 / 1000.0f) * 1.0f));
		physicsModels[i]->mLocation.SetScale(scale);
		physicsObjects[i].ResetPhysics();
	}
}