#include <vulkan/vulkan.h>
#include <imgui.h>

#include <numbers>

#include "Engine/Engine.h"

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
#include <glm/ext.hpp>

#include "Graphics/Conversions.h"
#include "Engine/Transform.h"

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

	//FRAMEBUFFER TEST
	//will need one for each eye?, container?
	Image fbImage;
	fbImage.SetArrayLayers(gGraphics->GetNumActiveViews());
	Image fbDepthImage;
	fbDepthImage.SetArrayLayers(gGraphics->GetNumActiveViews());
	fbImage.CreateVkImage(Graphics::GetDeafultColorFormat(), gGraphics->GetDesiredSize(), "Main FB Image");
	fbDepthImage.CreateVkImage(Graphics::GetDeafultDepthFormat(), gGraphics->GetDesiredSize(), "Main FB Depth Image");

	//convert to correct layout
	{
		auto buffer = gGraphics->AllocateGraphicsCommandBuffer();
		fbImage.SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		fbDepthImage.SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		gGraphics->EndGraphicsCommandBuffer(buffer);
	}

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
		mainPassClear[0].color.float32[0]	  = 0.0f;
		mainPassClear[0].color.float32[1]	  = 0.0f;
		mainPassClear[0].color.float32[2]	  = 0.0f;
		mainPassClear[0].color.float32[3]	  = 1.0f;
		mainPassClear[1].depthStencil.depth	  = 1.0f;
		mainPassClear[1].depthStencil.stencil = 0;
		mainRenderPass.SetClearColors(mainPassClear);
	}

	//FB write to these images, following the layout from mainRenderPass
	Framebuffer fb;
	fb.AddImage(&fbImage);
	fb.AddImage(&fbDepthImage);
	fb.Create(mainRenderPass, "Main Render FB");

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
	vrMirrorPass.GetMaterial(0).SetImages(fbImage, 0, 0);
#else
	//windows doesnt do multiview so just needs the non array version
	ssTest.Create("/Shaders/Screenspace/ImageSingle.frag.spv", "ScreenSpace ImageCopy");
#endif
	ssTest.GetMaterial(0).SetImages(fbImage, 0, 0);

	//Mesh Test
	Mesh meshTest;
	meshTest.LoadMesh(std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/MapleTree_5.fbx",
					  std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/");

	Mesh handMesh;
	handMesh.LoadMesh(std::string(WORK_DIR_REL) + "/Assets/handModel.fbx");

	struct MeshPCTest {
		glm::mat4 mWorld;
	} meshPC;
	struct MeshUniformTest {
		glm::mat4 mPV[2];
	} meshUniform;

	Buffer meshUniformBuffer;
	meshUniformBuffer.Create(BufferType::UNIFORM, sizeof(MeshUniformTest), "Mesh Uniform");

	Pipeline meshPipeline;
	meshPipeline.AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	meshPipeline.AddShader(std::string(WORK_DIR_REL) + "/Shaders/MeshTest.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPushConstantRange meshPCRange {};
	meshPCRange.size	   = sizeof(MeshPCTest);
	meshPCRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	meshPipeline.AddPushConstant(meshPCRange);
	{
		meshPipeline.vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		meshPipeline.vertexBinding.stride	 = sizeof(MeshVert);
		MeshVert temp;
		meshPipeline.vertexAttribute			 = std::vector<VkVertexInputAttributeDescription>(3);
		meshPipeline.vertexAttribute[0].location = 0;
		meshPipeline.vertexAttribute[0].format	 = VK_FORMAT_R32G32B32_SFLOAT;
		meshPipeline.vertexAttribute[0].offset	 = offsetof(MeshVert, mPos);
		meshPipeline.vertexAttribute[1].location = 1;
		meshPipeline.vertexAttribute[1].format	 = VK_FORMAT_R32G32B32A32_SFLOAT;
		meshPipeline.vertexAttribute[1].offset	 = offsetof(MeshVert, mColors[0]);
		meshPipeline.vertexAttribute[2].location = 2;
		meshPipeline.vertexAttribute[2].format	 = VK_FORMAT_R32G32_SFLOAT;
		meshPipeline.vertexAttribute[2].offset	 = offsetof(MeshVert, mUVs[0]);
	}

	MaterialBase meshTestBase;
	meshTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	//meshTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	meshTestBase.Create();
	meshPipeline.AddMaterialBase(&meshTestBase);
	MaterialBase meshImageTestBase;
	meshImageTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	meshImageTestBase.Create();
	meshPipeline.AddMaterialBase(&meshImageTestBase);
	meshPipeline.Create(mainRenderPass.GetRenderPass(), "Mesh");

	Material meshMaterial = meshTestBase.MakeMaterials()[0];
	meshMaterial.SetBuffers(meshUniformBuffer, 0, 0);

	Model modelTest1;
	modelTest1.SetMesh(&meshTest);
	modelTest1.SetMaterialBase(&meshImageTestBase);
	Model modelTest2;
	modelTest2.SetMesh(&meshTest);
	modelTest2.SetMaterialBase(&meshImageTestBase);
	Model controllerTest1;
	controllerTest1.SetMesh(&handMesh);
	controllerTest1.SetMaterialBase(&meshImageTestBase);
	Model controllerTest2;
	controllerTest2.SetMesh(&handMesh);
	controllerTest2.SetMaterialBase(&meshImageTestBase);
	Model worldBase;
	worldBase.SetMesh(&handMesh);
	worldBase.SetMaterialBase(&meshImageTestBase);

	Transform mModelTransforms[2];
	Transform mRootTransform;
	mModelTransforms[0].Set(glm::vec3(0, 0, 0), 1.0f, glm::vec3(-90, 0, 0), &mRootTransform);
	mModelTransforms[1].Set(glm::vec3(-5, 1, 0), 1.0f, glm::vec3(-90, 0, 0), &mRootTransform);
	Transform mCameraTransform;
	Transform mControllerTransforms[2];
	mControllerTransforms[0].SetParent(&mCameraTransform);
	mControllerTransforms[1].SetParent(&mCameraTransform);
	mControllerTransforms[0].SetScale(0);
	mControllerTransforms[1].SetScale(0);

	while(!gEngine->GetWindow()->ShouldClose()) {
		ZoneScoped;
		gEngine->GameLoop();
		gGraphics->StartNewFrame();
		VkCommandBuffer buffer = gGraphics->GetCurrentGraphicsCommandBuffer();

#if !defined(ENABLE_XR)
		static glm::vec3 camPos = glm::vec3(0, 0, 10);
		if(ImGui::Begin("Camera")) {
			ImGui::DragFloat3("Pos", glm::value_ptr(camPos), 0.1f, -999, 999);
			ImGui::End();
		}
#endif

		{
			glm::mat4 proj;
			glm::mat4 view;
			glm::mat4 world;

#if defined(ENABLE_XR)
			VRGraphics::View info;
			for(int i = 0; i < 2; i++) {
				gVrGraphics->GetEyePoseData(i, info);

				glm::mat4 translation;
				glm::mat4 rotation;

				translation = glm::translate(mCameraTransform.GetWorldMatrix(), info.mPos);
				rotation	= glm::mat4_cast(info.mRot);

				view = translation * rotation;
				view = glm::inverse(view);
				glm::translate(view, glm::vec3(5.0f, 0.0f, 5.0f));
				proj = glm::frustumRH_ZO(info.mFov.x, info.mFov.y, info.mFov.z, info.mFov.w, 0.1f, 100.0f);
				proj = glm::perspectiveFov(
					glm::radians(60.0f), (float)gGraphics->GetDesiredSize().mWidth, (float)gGraphics->GetDesiredSize().mHeight, 0.1f, 1000.0f);
				if(info.mFov.w != 0) {
					proj = glm::perspective(info.mFov.w - info.mFov.z, info.mFov.y / info.mFov.w, 0.1f, 1000.0f);
				}
				proj[1][1] *= -1.0f;
				meshUniform.mPV[i] = proj * view;
			}
#else
			const float time  = 0; //gGraphics->GetFrameCount() / 360.0f;
			const float scale = 10.0f;
			proj			  = glm::perspectiveFov(
				 glm::radians(60.0f), (float)gGraphics->GetDesiredSize().mWidth, (float)gGraphics->GetDesiredSize().mHeight, 0.1f, 1000.0f);
			//view			   = glm::lookAt(glm::vec3(sin(time) * scale, 0.0f, cos(time) * scale), glm::vec3(0), glm::vec3(0, 1, 0));
			view = glm::lookAt(camPos, glm::vec3(0), glm::vec3(0, 1, 0));
			proj[1][1] *= -1.0f;
			meshUniform.mPV[0] = proj * view;
			view			   = glm::lookAt(glm::vec3(1.0f, -20.0f, 1.0f), glm::vec3(0), glm::vec3(0, 1, 0));
			meshUniform.mPV[1] = proj * view;
#endif

			meshUniformBuffer.UpdateData(0, VK_WHOLE_SIZE, &meshUniform);
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
				ImGui::End();
			}
			const float time = gGraphics->GetFrameCount() / 5.0f;
			if(updateRoot) {
				mRootTransform.SetRotation(glm::vec3(0, -time, 0));
			}
#if defined(ENABLE_XR)
			if(updateControllers) {
				for(int i = 0; i < VRGraphics::Side::COUNT; i++) {
					VRGraphics::ControllerInfo info;
					gVrGraphics->GetHandInfo((VRGraphics::Side)i, info);
					glm::vec3 movement;
					if(info.mTrigger > 0) {
						glm::vec3 position = mCameraTransform.GetLocalPosition();
						movement		   = info.mLinearVelocity * info.mTrigger;
						mCameraTransform.SetPosition(position - movement);
					}
					if(info.mActive) {
						mControllerTransforms[i].SetPosition(info.mPose.mPos + movement);
						mControllerTransforms[i].SetRotation(info.mPose.mRot * glm::quat(glm::radians(glm::vec3(0, 180, 0))));
						mControllerTransforms[i].SetScale(0.1f);
					} else {
						mControllerTransforms[i].SetScale(0.0f);
					}
				}
			}
#endif
		}

		WorkManager::ImGuiTesting();

		const Framebuffer& framebuffer = fb;

		//mesh render test
		mainRenderPass.Begin(buffer, framebuffer);
		meshPipeline.Begin(buffer);
		//bind camera data
		vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline.GetLayout(), 0, 1, meshMaterial.GetSet(), 0, nullptr);
		//tree 1
		{
			meshPC.mWorld = mModelTransforms[0].GetWorldMatrix();
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			modelTest1.Render(buffer, meshPipeline.GetLayout());
		}
		//tree 2
		{
			meshPC.mWorld = mModelTransforms[1].GetWorldMatrix();
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			modelTest2.Render(buffer, meshPipeline.GetLayout());
		}
		//Hand 1
		{
			meshPC.mWorld = mControllerTransforms[0].GetWorldMatrix();
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			controllerTest1.Render(buffer, meshPipeline.GetLayout());
		}
		//Hand 2
		{
			meshPC.mWorld = mControllerTransforms[1].GetWorldMatrix();
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			controllerTest2.Render(buffer, meshPipeline.GetLayout());
		}
		//world base reference
		{
			meshPC.mWorld = glm::identity<glm::mat4>();
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			worldBase.Render(buffer, meshPipeline.GetLayout());
		}
		//world base reference
		{
			meshPC.mWorld = glm::translate(meshPC.mWorld, glm::vec3(5, 0, 0));
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			worldBase.Render(buffer, meshPipeline.GetLayout());
		}
		meshPipeline.End(buffer);
		mainRenderPass.End(buffer);

		fbImage.SetImageLayout(buffer,
							   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
							   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
							   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

#if defined(ENABLE_XR)
		//copy fbImage to computer view (using two eye version)
		vrMirrorPass.Render(buffer, gGraphics->GetCurrentFrameBuffer());
		//copy to headset views
		for(int i = 0; i < gGraphics->GetNumActiveViews(); i++) {
			Screenspace::PushConstant pc;
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

		gGraphics->EndFrame();
	}

	mRootTransform.Clear();

	meshPipeline.Destroy();
	meshImageTestBase.Destroy();
	meshTestBase.Destroy();
	meshTest.Destroy();
	meshUniformBuffer.Destroy();

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