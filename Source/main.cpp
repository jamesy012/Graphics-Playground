#include <vulkan/vulkan.h>
#include <imgui.h>

#include <numbers>

#include "Engine/Engine.h"
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

int main() {
	//vs code is annoying, doesnt clear the last output
	LOGGER::Log("--------------------------------\n");

	Graphics vulkanGraphics;

	Engine gameEngine;
	gameEngine.Startup(&vulkanGraphics);

	//we want to render with these outputs
	RenderPass mainRenderPass;
#if defined(ENABLE_XR)
	mainRenderPass.SetMultiViewSupport(true);
#endif
	mainRenderPass.AddColorAttachment(Graphics::GetDeafultColorFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR);
	mainRenderPass.AddDepthAttachment(Graphics::GetDeafultDepthFormat(), VK_ATTACHMENT_LOAD_OP_CLEAR);
	mainRenderPass.Create("Main Render RP");

	{
		std::vector<VkClearValue> mainPassClear(2);
		mainPassClear[0].color.float32[0] = 0.0f;
		mainPassClear[0].color.float32[1] = 0.0f;
		mainPassClear[0].color.float32[2] = 0.0f;
		mainPassClear[0].color.float32[3] = 1.0f;
		mainPassClear[1].depthStencil.depth = 1.0f;
		mainPassClear[1].depthStencil.stencil = 0;
		mainRenderPass.SetClearColors(mainPassClear);
	}

	//FRAMEBUFFER TEST
	Image fbImage;
	Image fbDepthImage;
	Framebuffer fb;
	auto CreateSizeDependentRenderObjects = [&]() {
		fb.Destroy();
		fbDepthImage.Destroy();
		fbImage.Destroy();
		fbImage.SetArrayLayers(gGraphics->GetNumActiveViews());
		fbDepthImage.SetArrayLayers(gGraphics->GetNumActiveViews());
		fbImage.CreateVkImage(Graphics::GetDeafultColorFormat(), gGraphics->GetDesiredSize(), true, "Main FB Image");
		fbDepthImage.CreateVkImage(Graphics::GetDeafultDepthFormat(), gGraphics->GetDesiredSize(), true, "Main FB Depth Image");

		//convert to correct layout
		{
			auto buffer = gGraphics->AllocateGraphicsCommandBuffer();
			fbImage.SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			fbDepthImage.SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			gGraphics->EndGraphicsCommandBuffer(buffer);
		}

		//FB write to these images, following the layout from mainRenderPass
		fb.AddImage(&fbImage);
		fb.AddImage(&fbDepthImage);
		fb.Create(mainRenderPass, "Main Render FB");
	};
	CreateSizeDependentRenderObjects();
	gGraphics->mResizeMessage.push_back(CreateSizeDependentRenderObjects);

	//SCREEN SPACE TEST
	//used to copy fbImage to backbuffer

	MaterialBase ssTestBase;
	ssTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	//ssTestBase.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	ssTestBase.Create();
	Screenspace ssTest;
	ssTest.AddMaterialBase(&ssTestBase);
	{
		std::vector<VkClearValue> ssClear(1);
		ssClear[0].color.float32[0] = 0.0f;
		ssClear[0].color.float32[1] = 0.0f;
		ssClear[0].color.float32[2] = 0.0f;
		ssClear[0].color.float32[3] = 1.0f;
		ssTest.SetClearColors(ssClear);
	}
#if defined(ENABLE_XR)
	//xr needs the array version
	ssTest.Create("/Shaders/Screenspace/ImageSingleArray.frag.spv", "ScreenSpace ImageCopy");
	//mirrors the left eye to the PC display
	Screenspace vrMirrorPass;
	vrMirrorPass.mAttachmentFormat = gGraphics->GetSwapchainFormat();
	vrMirrorPass.AddMaterialBase(&ssTestBase);
	vrMirrorPass.Create("/Shaders/Screenspace/ImageTwoArray.frag.spv", "ScreenSpace Mirror ImageCopy");
#else
	//windows doesnt do multiview so just needs the non array version
	ssTest.Create("/Shaders/Screenspace/ImageSingle.frag.spv", "ScreenSpace ImageCopy");
#endif
	auto SetupSSImages = [&]() {
#if defined(ENABLE_XR)
		vrMirrorPass.GetMaterial(0).SetImages(fbImage, 0, 0);
#endif
		ssTest.GetMaterial(0).SetImages(fbImage, 0, 0);
	};
	SetupSSImages();
	gGraphics->mResizeMessage.push_back(SetupSSImages);

	//Mesh Test
	Mesh meshTest;
	meshTest.LoadMesh(std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/MapleTree_5.fbx",
					  std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/");

	Mesh handMesh;
	handMesh.LoadMesh(std::string(WORK_DIR_REL) + "/Assets/handModel2.fbx");

	Mesh referenceMesh;
	referenceMesh.LoadMesh(std::string(WORK_DIR_REL) + "/Assets/5m reference.fbx");

	Mesh meshSceneTest;
	struct MeshTestHolder {
		std::string mName;
		std::string mFilePath;
		std::string mTexturePath;
	};
	// clang-format off
	const MeshTestHolder sceneMeshs[] = {
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
	// clang-format on
	const int numMesh = sizeof(sceneMeshs) / sizeof(MeshTestHolder);
	int selectedMesh = 0;
	const auto ChangeMesh = [&](int aIndex) {
		selectedMesh = aIndex;
		meshSceneTest.Destroy();
		meshSceneTest.LoadMesh(std::string(WORK_DIR_REL) + sceneMeshs[selectedMesh].mFilePath,
							   std::string(WORK_DIR_REL) + sceneMeshs[selectedMesh].mTexturePath);
	};
	//inital load
	ChangeMesh(selectedMesh);

	Pipeline meshPipeline;
	meshPipeline.AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	meshPipeline.AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	{
		meshPipeline.vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		meshPipeline.vertexBinding.stride = sizeof(MeshVert);
		meshPipeline.vertexAttribute = std::vector<VkVertexInputAttributeDescription>(4);
		meshPipeline.vertexAttribute[0].location = 0;
		meshPipeline.vertexAttribute[0].format = VK_FORMAT_R32G32B32_SFLOAT; //3
		meshPipeline.vertexAttribute[0].offset = offsetof(MeshVert, mPos);
		meshPipeline.vertexAttribute[1].location = 1;
		meshPipeline.vertexAttribute[1].format = VK_FORMAT_R32G32B32_SFLOAT; //6
		meshPipeline.vertexAttribute[1].offset = offsetof(MeshVert, mNorm);
		meshPipeline.vertexAttribute[2].location = 2;
		meshPipeline.vertexAttribute[2].format = VK_FORMAT_R32G32B32A32_SFLOAT; //10
		meshPipeline.vertexAttribute[2].offset = offsetof(MeshVert, mColors[0]);
		meshPipeline.vertexAttribute[3].location = 3;
		meshPipeline.vertexAttribute[3].format = VK_FORMAT_R32G32_SFLOAT; //12
		meshPipeline.vertexAttribute[3].offset = offsetof(MeshVert, mUVs[0]);
	}

	VkPushConstantRange meshPCRange {};
	meshPCRange.size = sizeof(MeshPCTest);
	meshPCRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	meshPipeline.AddPushConstant(meshPCRange);

	SceneData sceneData;

	Buffer sceneDataBuffer;
	sceneDataBuffer.Create(BufferType::UNIFORM, sizeof(SceneData), "Mesh Scene Uniform");

	MaterialBase meshTestBase;
	meshTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
	meshTestBase.Create();
	meshPipeline.AddMaterialBase(&meshTestBase);
	meshPipeline.AddBindlessTexture();
	meshPipeline.Create(mainRenderPass.GetRenderPass(), "Mesh");

	Material meshMaterial = meshTestBase.MakeMaterials()[0];
	meshMaterial.SetBuffers(sceneDataBuffer, 0, 0);

	Model modelTest1;
	modelTest1.SetMesh(&meshTest);
	Model modelTest2;
	modelTest2.SetMesh(&meshTest);
	Model controllerTest1;
	controllerTest1.SetMesh(&handMesh);
	Model controllerTest2;
	controllerTest2.SetMesh(&handMesh);
	Model worldBase;
	worldBase.SetMesh(&referenceMesh);

	Model modelSceneTest;
	modelSceneTest.SetMesh(&meshSceneTest);

	Transform mRootTransform;
	modelTest1.mLocation.Set(glm::vec3(0, 0, 0), 1.0f, &mRootTransform);
	modelTest2.mLocation.Set(glm::vec3(-5, 1, 0), 1.0f, &mRootTransform);
	FlyCamera camera;
	gEngine->SetMainCamera(&camera);
	camera.mTransform.SetPosition(glm::vec3(8.0f, 0.2f, 0.0f));
	controllerTest1.mLocation.SetParent(&camera.mTransform);
	controllerTest2.mLocation.SetParent(&camera.mTransform);
	controllerTest1.mLocation.SetScale(0);
	controllerTest2.mLocation.SetScale(0);
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

	while(!gEngine->GetWindow()->ShouldClose()) {
		ZoneScoped;
		gEngine->GameLoop();
		VkCommandBuffer buffer = gGraphics->GetCurrentGraphicsCommandBuffer();

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
			ImGui::End();
		}

		if(ImGui::Begin("Lighting")) {
			ImGui::DragFloat3("Direction", glm::value_ptr(sceneData.mDirectionalLight.mDirection), 0.01f, -1.0f, 1.0f);
			ImGui::ColorEdit3("Ambient", glm::value_ptr(sceneData.mDirectionalLight.mAmbient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(sceneData.mDirectionalLight.mDiffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(sceneData.mDirectionalLight.mSpecular));
		}
		ImGui::End();

		{
			glm::mat4 proj;
			glm::mat4 view;
			glm::mat4 world;

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
				//headMatrix	= glm::inverse(headMatrix);
			}
			VRGraphics::View info;
			for(int i = 0; i < 2; i++) {
				gVrGraphics->GetEyePoseData(i, info);

				glm::mat4 translation;
				glm::mat4 rotation;

				translation = glm::translate(glm::identity<glm::mat4>(), info.mPos);
				rotation = glm::mat4_cast(info.mRot);

				//view = headMatrix * (translation * rotation);
				view = camera.mTransform.GetWorldMatrix() * (translation * rotation);
				view = glm::inverse(view);
				glm::mat4 frustum =
					glm::frustum(tan(info.mFov.x) / 10, tan(info.mFov.y) / 10, tan(info.mFov.z) / 10, tan(info.mFov.w) / 10, 0.1f, 1000.0f);
				//glm::mat4 persp = glm::perspectiveFov(
				//	glm::radians(60.0f), (float)gGraphics->GetDesiredSize().mWidth, (float)gGraphics->GetDesiredSize().mHeight, 0.1f, 1000.0f);
				//if(info.mFov.w != 0) {
				//	persp = glm::perspective(info.mFov.w - info.mFov.z, info.mFov.y / info.mFov.w, 0.1f, 10000.0f);
				//}
				//proj = persp;
				proj = frustum;
				proj[1][1] *= -1.0f;

				sceneData.mPV[i] = proj * view;
				sceneData.mViewPos[i] = view[3];
			}
#else
			sceneData.mPV[0] = camera.GetViewProjMatrix();
			sceneData.mViewPos[0] = glm::vec4(camera.mTransform.GetWorldPosition(), 1);
#endif

			sceneDataBuffer.UpdateData(0, VK_WHOLE_SIZE, &sceneData);
		}

		{
			static bool updateRoot = true;
#if defined(ENABLE_XR)
			static bool updateControllers = true;
#endif
			if(ImGui::Begin("Scene")) {
				ImGui::Checkbox("UpdateRoot", &updateRoot);
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
				bool loaded = meshSceneTest.HasLoaded();
				ImGui::Checkbox("Has Loaded", &loaded);
				ImGui::EndDisabled();
			}
			ImGui::End();
			const float time = gEngine->GetTimeSinceStart() * 50;
			if(updateRoot) {
				mRootTransform.SetRotation(glm::vec3(0, -time, 0));

				modelTest2.mLocation.SetRotation(glm::vec3(0, time, 0));
			}
#if defined(ENABLE_XR)
			if(updateControllers) {
				Transform* controllerTransforms[2] = {&controllerTest1.mLocation, &controllerTest2.mLocation};
				for(int i = 0; i < VRGraphics::Side::COUNT; i++) {
					VRGraphics::ControllerInfo info;
					gVrGraphics->GetHandInfo((VRGraphics::Side)i, info);
					glm::vec3 movement;
					if(info.mTrigger > 0) {
						glm::vec3 position = camera.mTransform.GetLocalPosition();
						movement = (camera.mTransform.GetLocalRotation() * glm::vec4(info.mLinearVelocity, 1)) * info.mTrigger;
						camera.mTransform.SetPosition(position - movement);
					}
					if(info.mActive) {
						controllerTransforms[i]->SetPosition(info.mPose.mPos + movement);
						controllerTransforms[i]->SetRotation(info.mPose.mRot);
						controllerTransforms[i]->SetScale(1.0f);
					} else {
						controllerTransforms[i]->SetScale(0.0f);
					}
				}
			}
#endif
		}

		{
			ZoneScopedN("Render logic");

			const Framebuffer& framebuffer = fb;

			//mesh render test
			mainRenderPass.Begin(buffer, framebuffer);
			meshPipeline.Begin(buffer);
			//bind camera data
			vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline.GetLayout(), 0, 1, meshMaterial.GetSet(), 0, nullptr);
			if(gGraphics->GetMaterialManager()->IsInLargeArrayMode()) {
				vkCmdBindDescriptorSets(buffer,
										VK_PIPELINE_BIND_POINT_GRAPHICS,
										meshPipeline.GetLayout(),
										1,
										1,
										gGraphics->GetMaterialManager()->GetMainTextureSet(),
										0,
										nullptr);
			}
			//tree 1
			modelTest1.Render(buffer, meshPipeline.GetLayout());
			//tree 2
			modelTest2.Render(buffer, meshPipeline.GetLayout());
			//Hand 1
			controllerTest1.Render(buffer, meshPipeline.GetLayout());
			//Hand 2
			controllerTest2.Render(buffer, meshPipeline.GetLayout());
			//world base reference
			worldBase.Render(buffer, meshPipeline.GetLayout());
			modelSceneTest.Render(buffer, meshPipeline.GetLayout());
			meshPipeline.End(buffer);
			mainRenderPass.End(buffer);

			fbImage.SetImageLayout(buffer,
								   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
								   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			Screenspace::PushConstant pc;
			pc.mEyeIndex = 0;
			vkCmdPushConstants(buffer, ssTest.GetPipeline().GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Screenspace::PushConstant), &pc);
#if defined(ENABLE_XR)
			//copy fbImage to computer view (using two eye version)
			vrMirrorPass.Render(buffer, gGraphics->GetCurrentFrameBuffer());
			//copy to headset views
			for(int i = 0; i < gGraphics->GetNumActiveViews(); i++) {
				pc.mEyeIndex = i;
				vkCmdPushConstants(buffer, ssTest.GetPipeline().GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Screenspace::PushConstant), &pc);
				//copies fbImage to framebuffer
				ssTest.Render(buffer, gGraphics->GetCurrentXrFrameBuffer(i));
			}
#else
			ssTest.Render(buffer, gGraphics->GetCurrentFrameBuffer());
#endif

			fbImage.SetImageLayout(buffer,
								   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		}
		gGraphics->EndFrame();
	}

	mRootTransform.Clear();

	modelTest1.Destroy();
	modelTest2.Destroy();
	controllerTest2.Destroy();
	controllerTest1.Destroy();
	worldBase.Destroy();

	meshPipeline.Destroy();
	meshTestBase.Destroy();

	referenceMesh.Destroy();
	handMesh.Destroy();
	meshSceneTest.Destroy();
	meshTest.Destroy();
	sceneDataBuffer.Destroy();

	mainRenderPass.Destroy();
	fb.Destroy();
	fbDepthImage.Destroy();
	fbImage.Destroy();

#if defined(ENABLE_XR)
	vrMirrorPass.Destroy();
#endif
	ssTest.Destroy();
	ssTestBase.Destroy();

	gEngine->Shutdown();

	return 0;
}