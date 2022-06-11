#include <vulkan/vulkan.h>
#include <imgui.h>

#include "Graphics/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/Screenspace.h"
#include "Graphics/Image.h"
#include "Graphics/Material.h"

#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include <glm/ext.hpp>

int main() {
	//vs code is annoying, doesnt clear the last output
	LOG::LogLine("--------------------------------");

	Window window;
	window.Create(720, 720, "Graphics Playground");

	LOG::LogLine("Starting Graphics");
	Graphics gfx;
	gfx.StartUp();
	gfx.AddWindow(&window);
	LOG::LogLine("Initalize Graphics ");
	gfx.Initalize();
	LOG::LogLine("Graphics Initalized");

	//SCREEN SPACE TEST
	Image ssImage;
	ssImage.LoadImage(std::string(WORK_DIR_REL) + "WorkDir/Assets/quanternius/tree/MapleTree_Bark.png", VK_FORMAT_R8G8B8A8_UNORM);
	Image ssImage2;
	ssImage2.LoadImage(std::string(WORK_DIR_REL) + "WorkDir/Assets/quanternius/tree/MapleTree_Leaves.png", VK_FORMAT_R8G8B8A8_UNORM);

	MaterialBase ssTestBase;
	ssTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	//ssTestBase.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	ssTestBase.Create();
	Screenspace ssTest;
	ssTest.AddMaterialBase(&ssTestBase);
	ssTest.Create("WorkDir/Shaders/Screenspace/Image.frag.spv", "ScreenSpace ImageCopy");
	ssTest.GetMaterial(0).SetImages(ssImage, 0, 0);
	//ssTest.GetMaterial(0).SetImages(ssImage2, 1, 0);

	//Mesh Test
	Mesh meshTest;
	meshTest.LoadMesh(std::string(WORK_DIR_REL) + "WorkDir/Assets/quanternius/tree/MapleTree_5.fbx");

	struct MeshPCTest {
		glm::mat4 mWorld;
	} meshPC;
	struct MeshUniformTest {
		glm::mat4 mPV;
	} meshUniform;

	Buffer meshUniformBuffer;
	meshUniformBuffer.Create(BufferType::UNIFORM, sizeof(MeshUniformTest), "Mesh Uniform");

	Pipeline meshPipeline;
	meshPipeline.AddShader(std::string(WORK_DIR_REL) + "WorkDir/Shaders/MeshTest.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	meshPipeline.AddShader(std::string(WORK_DIR_REL) + "WorkDir/Shaders/MeshTest.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPushConstantRange meshPCRange {};
	meshPCRange.size	   = sizeof(MeshPCTest);
	meshPCRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	meshPipeline.AddPushConstant(meshPCRange);
	{
		meshPipeline.vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		meshPipeline.vertexBinding.stride	 = sizeof(MeshVert);

		meshPipeline.vertexAttribute			 = std::vector<VkVertexInputAttributeDescription>(2);
		meshPipeline.vertexAttribute[0].location = 0;
		meshPipeline.vertexAttribute[0].format	 = VK_FORMAT_R32G32B32_SFLOAT;
		meshPipeline.vertexAttribute[0].offset	 = offsetof(MeshVert, mPos);
		meshPipeline.vertexAttribute[1].location = 1;
		meshPipeline.vertexAttribute[1].format	 = VK_FORMAT_R32G32_SFLOAT;
		meshPipeline.vertexAttribute[1].offset	 = offsetof(MeshVert, mUV[0]);
	}

	RenderPass meshRenderPass;
	meshRenderPass.Create("Mesh RP");
	MaterialBase meshTestBase;
	meshTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	meshTestBase.Create();
	meshPipeline.SetMaterialBase(&meshTestBase);
	meshPipeline.Create(meshRenderPass.GetRenderPass(), "Mesh");
	Material meshMaterial = meshTestBase.AllocateMaterials()[0];
	meshMaterial.SetBuffers(meshUniformBuffer, 0, 0);

	while(!window.ShouldClose()) {
		window.Update();

		{
			glm::mat4 proj;
			glm::mat4 view;
			glm::mat4 world;

			proj			  = glm::perspectiveFov(glm::radians(60.0f), 720.0f, 720.0f, 0.1f, 1000.0f);
			const float time  = gGraphics->GetFrameCount() / 360.0f;
			const float scale = 10.0f;
			view			  = glm::lookAt(glm::vec3(sin(time) * scale, 0.0f, cos(time) * scale), glm::vec3(0), glm::vec3(0, 1, 0));

			meshUniform.mPV = proj * view;

			meshUniformBuffer.UpdateData(0, VK_WHOLE_SIZE, &meshUniform);
		}

		gfx.StartNewFrame();

		VkCommandBuffer buffer = gfx.GetCurrentGraphicsCommandBuffer();
#if defined(ENABLE_XR)
		const Framebuffer* frameBuffers[3] = {&gfx.GetCurrentFrameBuffer(), &gfx.GetCurrentXrFrameBuffer(0), &gfx.GetCurrentXrFrameBuffer(1)};
		for(int i = 0; i < 3; i++) {
#else
		const Framebuffer* frameBuffers[1] = {&gfx.GetCurrentFrameBuffer()};
		const int i						   = 0;
		{
#endif
			const Framebuffer& framebuffer = *frameBuffers[i];

			ssTest.Render(buffer, framebuffer);

			//mesh render test
			meshRenderPass.Begin(buffer, framebuffer);
			meshPipeline.Begin(buffer);
			vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline.GetLayout(), 0, 1, meshMaterial.GetSet(), 0, nullptr);
			{
				meshPC.mWorld = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 5, 0));
				meshPC.mWorld = glm::rotate(meshPC.mWorld, glm::radians(90.0f), glm::vec3(1, 0, 0));
				vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
				meshTest.QuickTempRender(buffer);
			}
			{
				meshPC.mWorld = glm::translate(glm::identity<glm::mat4>(), glm::vec3(-5, 8, 0));
				meshPC.mWorld = glm::rotate(meshPC.mWorld, glm::radians(90.0f), glm::vec3(1, 0, 0));
				vkCmdPushConstants(buffer, meshPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPCTest), &meshPC);
				meshTest.QuickTempRender(buffer);
			}
			meshPipeline.End(buffer);
			meshRenderPass.End(buffer);
		}

		gfx.EndFrame();
	}

	meshPipeline.Destroy();
	meshTestBase.Destroy();
	meshRenderPass.Destroy();
	meshTest.Destroy();
	meshUniformBuffer.Destroy();

	ssTest.Destroy();
	ssTestBase.Destroy();
	ssImage.Destroy();
	ssImage2.Destroy();

	gfx.Destroy();

	window.Destroy();

	return 0;
}