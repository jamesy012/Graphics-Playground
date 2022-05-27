#include "Graphics/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/Swapchain.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Framebuffer.h"
#include "Graphics/Image.h"

#include <vulkan/vulkan.h>
#include <imgui.h>

int main() {

	//vs code is annoying, doesnt clear the last output
	LOG::LogLine("--------------------------------");

	Window window;
	window.Create(720, 720, "Graphics Playground");

	LOG::LogLine("Starting Graphics");
	Graphics gfx;
	gfx.StartUp();
	gfx.AddWindow(&window);
	gfx.Initalize();

	Image img;
	img.CreateVkImage(gfx.GetMainFormat(), gfx.GetMainSwapchain()->GetSize());
	img.Destroy();

    while (!window.ShouldClose()) {
        window.Update();

		gfx.StartNewFrame();

		ImGui::ShowDemoWindow();
		
		gfx.EndFrame();
    }

	gfx.Destroy();

	return 0;
}