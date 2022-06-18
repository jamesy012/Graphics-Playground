#include <vulkan/vulkan.h>
#include <imgui.h>

#include "Engine/Engine.h"

#include "Engine/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/Framebuffer.h"

#include "Graphics/Screenspace.h"
#include "Graphics/Image.h"
#include "Graphics/Material.h"

#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include <glm/ext.hpp>

#include "Graphics/Conversions.h"
#include "Engine/Transform.h"

#if defined(ENABLE_XR)
#	include "Graphics/VRGraphics.h"
#endif

int main() {
	//vs code is annoying, doesnt clear the last output
	LOGGER::Log("--------------------------------\n");

	Graphics vulkanGraphics;

	Engine gameEngine;
	gameEngine.Startup(&vulkanGraphics);

	//FRAMEBUFFER TEST
	//will need one for each eye?, container?
	Image fbImage;
	fbImage.SetArrayLayers(2);
	Image fbDepthImage;
	fbDepthImage.SetArrayLayers(2);
	fbImage.CreateVkImage(Graphics::GetDeafultColorFormat(), {720, 720}, "Main FB Image");
	fbDepthImage.CreateVkImage(Graphics::GetDeafultDepthFormat(), {720, 720}, "Main FB Depth Image");

	//convert to correct layout
	{
		auto buffer = gGraphics->AllocateGraphicsCommandBuffer();
		fbImage.SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		fbDepthImage.SetImageLayout(buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		gGraphics->EndGraphicsCommandBuffer(buffer);
	}

	//we want to render with these outputs
	RenderPass mainRenderPass;
	mainRenderPass.SetMultiViewSupport(true);
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

	//Image ssImage;
	//ssImage.LoadImage(std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/MapleTree_Bark.png", Graphics::GetDeafultColorFormat());
	//Image ssImage2;
	//ssImage2.LoadImage(std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/MapleTree_Leaves.png", Graphics::GetDeafultColorFormat());

	MaterialBase ssTestBase;
	ssTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	//ssTestBase.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	ssTestBase.Create();
	Screenspace ssTest;
	ssTest.AddMaterialBase(&ssTestBase);
	ssTest.Create("/Shaders/Screenspace/ImageSingleArray.frag.spv", "ScreenSpace ImageCopy");
	ssTest.GetMaterial(0).SetImages(fbImage, 0, 0);
#if defined(ENABLE_XR)
	//mirrors the left eye to the display
	Screenspace vrMirrorPass;
	vrMirrorPass.mAttachmentFormat = gGraphics->GetSwapchainFormat();
	vrMirrorPass.AddMaterialBase(&ssTestBase);
	vrMirrorPass.Create("/Shaders/Screenspace/ImageMirror.frag.spv", "ScreenSpace ImageCopy");
	vrMirrorPass.GetMaterial(0).SetImages(fbImage, 0, 0);
#endif
	//ssTest.GetMaterial(0).SetImages(ssImage2, 1, 0);

	//Mesh Test
	Mesh meshTest;
	meshTest.LoadMesh(std::string(WORK_DIR_REL) + "/Assets/quanternius/tree/MapleTree_5.fbx");

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
		meshPipeline.vertexAttribute			 = std::vector<VkVertexInputAttributeDescription>(2);
		meshPipeline.vertexAttribute[0].location = 0;
		meshPipeline.vertexAttribute[0].format	 = VK_FORMAT_R32G32B32_SFLOAT;
		meshPipeline.vertexAttribute[0].offset	 = offsetof(MeshVert, mPos);
		meshPipeline.vertexAttribute[1].location = 1;
		meshPipeline.vertexAttribute[1].format	 = VK_FORMAT_R32G32_SFLOAT;
		meshPipeline.vertexAttribute[1].offset	 = offsetof(MeshVert, mUVs[0]);
	}

	MaterialBase meshTestBase;
	meshTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	meshTestBase.Create();
	meshPipeline.SetMaterialBase(&meshTestBase);
	meshPipeline.Create(mainRenderPass.GetRenderPass(), "Mesh");

	Material meshMaterial = meshTestBase.MakeMaterials()[0];
	meshMaterial.SetBuffers(meshUniformBuffer, 0, 0);

	Transform mModelTransforms[2];
	Transform mRootTransform;
	mModelTransforms[0].Set(glm::vec3(0, 5, 0), 1.0f, glm::vec3(90, 0, 0), &mRootTransform);
	mModelTransforms[1].Set(glm::vec3(-5, 8, 0), 1.0f, glm::vec3(90, 0, 0), &mRootTransform);

	while(!gEngine->GetWindow()->ShouldClose()) {
		gEngine->GetWindow()->Update();

		{
			glm::mat4 proj;
			glm::mat4 view;
			glm::mat4 world;

#if defined(ENABLE_XR)
			VRGraphics::GLMViewInfo info;
			for(int i = 0; i < 2; i++) {
				gGraphics->GetVrGraphics()->GetEyePoseData(i, info);

				glm::mat4 translation;
				glm::mat4 rotation;

				translation = glm::translate(glm::identity<glm::mat4>(), info.mPos);
				translation = glm::translate(glm::identity<glm::mat4>(), info.mPos);
				rotation	= glm::mat4_cast(info.mRot);

				view = translation * rotation;
				view = glm::inverse(view);
				glm::translate(view, glm::vec3(5.0f, 0.0f, 5.0f));
				proj = glm::frustumRH_ZO(info.mFov.x, info.mFov.y, info.mFov.z, info.mFov.w, 0.1f, 100.0f);
				proj = glm::perspectiveFov(glm::radians(60.0f), 720.0f, 720.0f, 0.1f, 1000.0f);
				if(info.mFov.w != 0) {
					proj = glm::perspective(info.mFov.w - info.mFov.z, info.mFov.y / info.mFov.w, 0.1f, 1000.0f);
				}
				meshUniform.mPV[i] = proj * view;
			}
#else
			const float time   = 0; //gGraphics->GetFrameCount() / 360.0f;
			const float scale  = 10.0f;
			proj			   = glm::perspectiveFov(glm::radians(60.0f), 720.0f, 720.0f, 0.1f, 1000.0f);
			view			   = glm::lookAt(glm::vec3(sin(time) * scale, 0.0f, cos(time) * scale), glm::vec3(0), glm::vec3(0, 1, 0));
			meshUniform.mPV[0] = proj * view;
			view			   = glm::lookAt(glm::vec3(1.0f, -20.0f, 1.0f), glm::vec3(0), glm::vec3(0, 1, 0));
			meshUniform.mPV[1] = proj * view;
#endif

			meshUniformBuffer.UpdateData(0, VK_WHOLE_SIZE, &meshUniform);
		}

		{
			const float time = gGraphics->GetFrameCount() / 5.0f;
			//mRootTransform.SetRotation(glm::vec3(0, -time, 0));
		}

		gGraphics->StartNewFrame();

		VkCommandBuffer buffer = gGraphics->GetCurrentGraphicsCommandBuffer();

		const Framebuffer& framebuffer = fb;

		//mesh render test
		mainRenderPass.Begin(buffer, framebuffer);
		meshPipeline.Begin(buffer);
		//bind camera data
		vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline.GetLayout(), 0, 1, meshMaterial.GetSet(), 0, nullptr);
		//tree 1
		{
			//meshPC.mWorld = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 5, 0));
			//meshPC.mWorld = glm::rotate(meshPC.mWorld, glm::radians(90.0f), glm::vec3(1, 0, 0));
			meshPC.mWorld = mModelTransforms[0].GetWorldMatrix();
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			meshTest.QuickTempRender(buffer);
		}
		//tree 2
		{
			//meshPC.mWorld = glm::translate(glm::identity<glm::mat4>(), glm::vec3(-5, 8, 0));
			//meshPC.mWorld = glm::rotate(meshPC.mWorld, glm::radians(90.0f), glm::vec3(1, 0, 0));
			meshPC.mWorld = mModelTransforms[1].GetWorldMatrix();
			vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
			meshTest.QuickTempRender(buffer);
		}
		meshPipeline.End(buffer);
		mainRenderPass.End(buffer);

		fbImage.SetImageLayout(buffer,
							   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
							   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
							   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

#if defined(ENABLE_XR)
		const Framebuffer* frameBuffers[3] = {
			&gGraphics->GetCurrentXrFrameBuffer(0),
			&gGraphics->GetCurrentXrFrameBuffer(1),
			&gGraphics->GetCurrentFrameBuffer(),
		};
		for(int i = 0; i < 2; i++) {
#else
		const Framebuffer* frameBuffers[1] = {&gGraphics->GetCurrentFrameBuffer()};
		const int i = 0;
		{
#endif
			Screenspace::PushConstant pc;
			pc.mEyeIndex  = i;
			vkCmdPushConstants(buffer, ssTest.GetPipeline().GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Screenspace::PushConstant), &pc);
			//copies fbImage to framebuffer
			ssTest.Render(buffer, *frameBuffers[i]);
#if defined(ENABLE_XR)
			vrMirrorPass.Render(buffer, *frameBuffers[i + 1]);
#endif
		}

		fbImage.SetImageLayout(buffer,
							   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
							   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
							   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		gGraphics->EndFrame();
	}

	mRootTransform.Clear();

	meshPipeline.Destroy();
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