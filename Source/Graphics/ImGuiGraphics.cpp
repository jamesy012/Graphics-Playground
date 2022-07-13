#include "ImGuiGraphics.h"

#include <backends/imgui_impl_glfw.h>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "Graphics.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "Image.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "Material.h"

#include "PlatformDebug.h"

ImGuiGraphics* gImGuiGraphics = nullptr;

struct ImGuiPushConstant {
	glm::vec2 mScale;
	glm::vec2 mUv;
} gImGuiPushConstant;

ImGuiContext* gImGuiContext;

Image gImGuiFontImage;
Pipeline gImGuiPipeline;
Buffer gImGuiVertBuffer;
Buffer gImGuiIndexBuffer;
MaterialBase gImGuiFontMaterialBase;
Material gImGuiFontMaterial;

void ImGuiGraphics::Create(GLFWwindow* aWindow, const RenderPass& aRenderPass) {
	ZoneScoped;
	ASSERT(gImGuiGraphics == nullptr);
	gImGuiGraphics = this;
	ASSERT(aWindow != nullptr);
	ASSERT(gImGuiContext == nullptr);
	gImGuiContext = ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(aWindow, true);

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigWindowsMoveFromTitleBarOnly = true;

	const VkExtent2D& swapSizeExtent = gGraphics->GetMainSwapchain()->GetSize();
	ImVec2 swapSize = ImVec2(swapSizeExtent.width, swapSizeExtent.height);
	ImVec2 swapScale;
	glfwGetWindowContentScale(aWindow, &swapScale.x, &swapScale.y);
	{
		io.DisplaySize = swapSize;
		io.DisplayFramebufferScale = swapScale;
	}

	unsigned char* fontData;
	int fontWidth;
	int fontHeight;
	int fontBytes;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &fontWidth, &fontHeight, &fontBytes);
	const int bufferSize = fontWidth * fontHeight * sizeof(unsigned char) * fontBytes;
	ImageSize size(fontWidth, fontHeight);

	Buffer fontBuffer;
	fontBuffer.CreateFromData(BufferType::STAGING, bufferSize, fontData, "ImGui Font Data");

	gImGuiFontImage.CreateFromBuffer(fontBuffer, true, VK_FORMAT_R8G8B8A8_UNORM, size, "ImGui Font Image");

	//fontBuffer.Destroy();

	gImGuiPipeline.AddShader(std::string(WORK_DIR_REL) + "/Shaders/imgui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	gImGuiPipeline.AddShader(std::string(WORK_DIR_REL) + "/Shaders/imgui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

	gImGuiFontMaterialBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	gImGuiFontMaterialBase.Create("ImGui Material Layout");
	gImGuiPipeline.AddMaterialBase(&gImGuiFontMaterialBase);

	VkPushConstantRange pushRange = {};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.size = sizeof(ImGuiPushConstant);
	gImGuiPipeline.AddPushConstant(pushRange);

	//temp
	{
		gImGuiPipeline.vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		gImGuiPipeline.vertexBinding.stride = sizeof(ImDrawVert);

		gImGuiPipeline.vertexAttribute = std::vector<VkVertexInputAttributeDescription>(3);
		gImGuiPipeline.vertexAttribute[0].format = VK_FORMAT_R32G32_SFLOAT;
		gImGuiPipeline.vertexAttribute[0].offset = offsetof(ImDrawVert, pos);
		gImGuiPipeline.vertexAttribute[1].location = 1;
		gImGuiPipeline.vertexAttribute[1].format = VK_FORMAT_R32G32_SFLOAT;
		gImGuiPipeline.vertexAttribute[1].offset = offsetof(ImDrawVert, uv);
		gImGuiPipeline.vertexAttribute[2].location = 2;
		gImGuiPipeline.vertexAttribute[2].format = VK_FORMAT_R8G8B8A8_UNORM;
		gImGuiPipeline.vertexAttribute[2].offset = offsetof(ImDrawVert, col);
	}

	gImGuiPipeline.Create(aRenderPass.GetRenderPass(), "ImGui Pipeline");

	gImGuiVertBuffer.Create(BufferType::VERTEX, 0, "ImGui Vertex Buffer");
	gImGuiIndexBuffer.Create(BufferType::INDEX, 0, "ImGui Index Buffer");

	{
		gImGuiFontMaterial.Create(&gImGuiFontMaterialBase, "ImGui Font Material");
		gImGuiFontMaterial.SetImages(gImGuiFontImage, 0, 0);
	}
}

void ImGuiGraphics::StartNewFrame() {
	ZoneScoped;
	ImGui::NewFrame();
	ImGui_ImplGlfw_NewFrame();

	//ImGuiIO& io			   = ImGui::GetIO();
	//const VkExtent2D& swapSizeExtent = gGraphics->GetMainSwapchain()->GetSize();
	//ImVec2 swapSize					 = ImVec2(swapSizeExtent.width, swapSizeExtent.height);
	//io.DisplaySize = swapSize;

	//temp demo window
	ImGui::ShowDemoWindow();
}

void ImGuiGraphics::RenderImGui(const VkCommandBuffer aBuffer, const RenderPass& aRenderPass, const Framebuffer& aFramebuffer) {
	ZoneScoped;
	ASSERT(aBuffer != VK_NULL_HANDLE);
	ASSERT(gImGuiContext != nullptr);

	//end imgui
	ImGui::EndFrame();
	ImGui::Render();

	//render imgui
	ImDrawData* imDrawData = ImGui::GetDrawData();

	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
	gImGuiVertBuffer.Resize(vertexBufferSize, false, "ImGui Vertex Buffer");
	gImGuiIndexBuffer.Resize(indexBufferSize, false, "ImGui Index Buffer");

	if(vertexBufferSize != 0 && indexBufferSize != 0) {
		ImDrawVert* vertexData = (ImDrawVert*)gImGuiVertBuffer.Map();
		ImDrawIdx* indexData = (ImDrawIdx*)gImGuiIndexBuffer.Map();

		for(int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vertexData, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(indexData, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vertexData += cmd_list->VtxBuffer.Size;
			indexData += cmd_list->IdxBuffer.Size;
		}

		gImGuiVertBuffer.Flush();
		gImGuiIndexBuffer.Flush();

		gImGuiVertBuffer.UnMap();
		gImGuiIndexBuffer.UnMap();
	}

	// mRenderPass.Begin(aBuffer, gRenderTarget.GetFb());

	// vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
	// &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gImGuiPipeline.GetPipeline());

	// VkViewport viewport = vks::initializers::viewport(ImGui::GetIO().DisplaySize.x,
	// ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f); vkCmdSetViewport(aBuffer, 0, 1, &viewport);

	aRenderPass.Begin(aBuffer, aFramebuffer);

	if(vertexBufferSize != 0 && indexBufferSize != 0) {
		// UI scale and translate via push constants
		ImVec2 scale = ImVec2(2.0f / imDrawData->DisplaySize.x, 2.0f / imDrawData->DisplaySize.y);
		gImGuiPushConstant.mScale = glm::vec2(scale.x, scale.y);
		gImGuiPushConstant.mUv = glm::vec2(-1.0f - imDrawData->DisplayPos.x * scale.x, -1.0f - imDrawData->DisplayPos.y * scale.y);
		vkCmdPushConstants(aBuffer, gImGuiPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ImGuiPushConstant), &gImGuiPushConstant);

		vkCmdBindDescriptorSets(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gImGuiPipeline.GetLayout(), 0, 1, gImGuiFontMaterial.GetSet(), 0, 0);

		// Render commands
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if(imDrawData->CmdListsCount > 0) {

			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(aBuffer, 0, 1, gImGuiVertBuffer.GetBufferRef(), offsets);
			vkCmdBindIndexBuffer(aBuffer, gImGuiIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

			VkViewport viewport = {};
			viewport.width = imDrawData->DisplaySize.x * imDrawData->FramebufferScale.x;
			viewport.height = imDrawData->DisplaySize.y * imDrawData->FramebufferScale.y;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(aBuffer, 0, 1, &viewport);

			// Will project scissor/clipping rectangles into framebuffer space
			ImVec2 clip_off = imDrawData->DisplayPos; // (0,0) unless using multi-viewports
			ImVec2 clip_scale = imDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

			for(int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for(int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					{
						// Project scissor/clipping rectangles into framebuffer space
						ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
						ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

						// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
						if(clip_min.x < 0.0f) {
							clip_min.x = 0.0f;
						}
						if(clip_min.y < 0.0f) {
							clip_min.y = 0.0f;
						}
						if(clip_max.x > aFramebuffer.GetSize().width) {
							clip_max.x = (float)aFramebuffer.GetSize().width;
						}
						if(clip_max.y > aFramebuffer.GetSize().height) {
							clip_max.y = (float)aFramebuffer.GetSize().height;
						}
						if(clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
							continue;

						// Apply scissor/clipping rectangle
						VkRect2D scissor;
						scissor.offset.x = (int32_t)(clip_min.x);
						scissor.offset.y = (int32_t)(clip_min.y);
						scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
						scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
						vkCmdSetScissor(aBuffer, 0, 1, &scissor);
					}
					vkCmdDrawIndexed(aBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}
	}

	aRenderPass.End(aBuffer);
}

void ImGuiGraphics::Destroy() {
	ZoneScoped;
	ASSERT(gImGuiGraphics != nullptr);
	gImGuiFontMaterialBase.Destroy();
	gImGuiFontImage.Destroy();
	gImGuiPipeline.Destroy();
	gImGuiVertBuffer.Destroy();
	gImGuiIndexBuffer.Destroy();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(gImGuiContext);
	gImGuiContext = nullptr;

	gImGuiGraphics = nullptr;
}
