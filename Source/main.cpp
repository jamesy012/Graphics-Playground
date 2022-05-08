#include "Graphics/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/Swapchain.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Framebuffer.h"
#include "Graphics/Image.h"

#include <vulkan/vulkan.h>

int main() {

	//vs code is annoying, doesnt clear the last output
	LOG::LogLine("--------------------------------");

	Window window;
	window.Create(500, 500, "Graphics Playground");

	LOG::LogLine("Starting Graphics");
	Graphics gfx;
	gfx.StartUp();
	gfx.AddWindow(&window);
	gfx.Initalize();

	Image img;
	img.CreateVkImage(gfx.GetMainFormat(), gfx.GetMainSwapchain()->GetSize());
	img.Destroy();

	RenderPass test;
	test.Create();

	Framebuffer fbTest[3];
	for(int i = 0;i<3;i++) {
		fbTest[i].Create(gfx.GetMainSwapchain()->GetImage(i), test);
	}


    while (!window.ShouldClose()) {
        window.Update();

		gfx.StartNewFrame();

		test.Begin(gfx.GetCurrentGraphicsCommandBuffer(), fbTest[gfx.GetCurrentImageIndex()]);

		test.End(gfx.GetCurrentGraphicsCommandBuffer());
		
		gfx.EndFrame();
    }

	gfx.Destroy();

	return 0;
}