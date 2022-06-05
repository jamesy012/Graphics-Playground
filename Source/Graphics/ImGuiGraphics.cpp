#include "ImGuiGraphics.h"

#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <glm/glm.hpp>

#include "Graphics.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "Image.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "Material.h"

#include "PlatformDebug.h"

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
	ASSERT(aWindow != nullptr);
	ASSERT(gImGuiContext == nullptr);
	gImGuiContext = ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(aWindow, true);

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigWindowsMoveFromTitleBarOnly = true;

	const VkExtent2D& swapSizeExtent = gGraphics->GetMainSwapchain()->GetSize();
	ImVec2 swapSize					 = ImVec2(swapSizeExtent.width, swapSizeExtent.height);

	{
		io.DisplaySize			   = swapSize;
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
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

	gImGuiFontImage.CreateFromBuffer(fontBuffer, VK_FORMAT_R8G8B8A8_UNORM, size, "ImGui Font Image");

	fontBuffer.Destroy();

	gImGuiPipeline.AddShader(std::string(WORK_DIR_REL) + "WorkDir/Shaders/imgui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	gImGuiPipeline.AddShader(std::string(WORK_DIR_REL) + "WorkDir/Shaders/imgui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);

	gImGuiFontMaterialBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	gImGuiFontMaterialBase.Create("ImGui Material Layout");
	gImGuiPipeline.SetMaterialBase(&gImGuiFontMaterialBase);

	VkPushConstantRange pushRange = {};
	pushRange.stageFlags		  = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.size				  = sizeof(ImGuiPushConstant);
	gImGuiPipeline.AddPushConstant(pushRange);

	gImGuiPipeline.Create(aRenderPass.GetRenderPass(), "ImGui Pipeline");

	gImGuiVertBuffer.Create(BufferType::VERTEX, 0, "ImGui Vertex Buffer");
	gImGuiIndexBuffer.Create(BufferType::INDEX, 0, "ImGui Index Buffer");

	{
		gImGuiFontMaterial.Create(&gImGuiFontMaterialBase, "ImGui Font Material");
		gImGuiFontMaterial.SetImage(gImGuiFontImage);
	}
}

void ImGuiGraphics::StartNewFrame() {
	ImGui::NewFrame();
	ImGui_ImplGlfw_NewFrame();

	//temp demo window
	ImGui::ShowDemoWindow();
}

void ImGuiGraphics::RenderImGui(const VkCommandBuffer aBuffer, const RenderPass& aRenderPass, const Framebuffer& aFramebuffer) {
	ASSERT(aBuffer != VK_NULL_HANDLE);
	ASSERT(gImGuiContext != nullptr);

	//end imgui
	ImGui::EndFrame();
	ImGui::Render();

	//render imgui
	ImGuiIO& io			   = ImGui::GetIO();
	ImDrawData* imDrawData = ImGui::GetDrawData();

	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize  = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
	gImGuiVertBuffer.Resize(vertexBufferSize, false, "ImGui Vertex Buffer");
	gImGuiIndexBuffer.Resize(indexBufferSize, false, "ImGui Index Buffer");

	if(vertexBufferSize != 0 && indexBufferSize != 0) {
		ImDrawVert* vertexData = (ImDrawVert*)gImGuiVertBuffer.Map();
		ImDrawIdx* indexData   = (ImDrawIdx*)gImGuiIndexBuffer.Map();

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
		gImGuiPushConstant.mScale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		gImGuiPushConstant.mUv	  = glm::vec2(-1.0f);
		vkCmdPushConstants(aBuffer, gImGuiPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ImGuiPushConstant), &gImGuiPushConstant);

		vkCmdBindDescriptorSets(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gImGuiPipeline.GetLayout(), 0, 1, gImGuiFontMaterial.GetSet(), 0, 0);

		// Render commands
		int32_t vertexOffset = 0;
		int32_t indexOffset	 = 0;

		if(imDrawData->CmdListsCount > 0) {

			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(aBuffer, 0, 1, gImGuiVertBuffer.GetBufferRef(), offsets);
			vkCmdBindIndexBuffer(aBuffer, gImGuiIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

			VkViewport viewport = {};
			viewport.width		= gGraphics->GetMainSwapchain()->GetSize().width;
			viewport.height		= gGraphics->GetMainSwapchain()->GetSize().height;
			viewport.maxDepth	= 1.0f;
			vkCmdSetViewport(aBuffer, 0, 1, &viewport);

			for(int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for(int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x	  = std::max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y	  = std::max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width  = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					vkCmdSetScissor(aBuffer, 0, 1, &scissorRect);
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
	gImGuiFontMaterialBase.Destroy();
	gImGuiFontImage.Destroy();
	gImGuiPipeline.Destroy();
	gImGuiVertBuffer.Destroy();
	gImGuiIndexBuffer.Destroy();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(gImGuiContext);
	gImGuiContext = nullptr;
}
