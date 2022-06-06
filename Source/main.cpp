#include <vulkan/vulkan.h>
#include <imgui.h>

#include "Graphics/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/Screenspace.h"

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

	Screenspace ssTest;
	ssTest.Create("WorkDir/Shaders/Screenspace/Image.frag.spv", "ScreenSpace ImageCopy");

	while(!window.ShouldClose()) {
		window.Update();

		gfx.StartNewFrame();

		ssTest.Render(gfx.GetCurrentGraphicsCommandBuffer(), gfx.GetCurrentFrameBuffer());

		gfx.EndFrame();
	}

	ssTest.Destroy();

	gfx.Destroy();

	window.Destroy();

	return 0;
}