#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;
class RenderPass;
class Framebuffer;

//Using the ImGui Library
//it is global context bases so this should only be needed for setup/frame management
class ImGuiGraphics {
public:
	void Create(GLFWwindow* aWindow, const RenderPass& aRenderPass);

	void StartNewFrame();

	void RenderImGui(const VkCommandBuffer aBuffer, const RenderPass& aRenderPass, const Framebuffer& aFramebuffer);

	void Destroy();
};

extern ImGuiGraphics* gImGuiGraphics;