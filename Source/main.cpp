#include "Graphics/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/RenderPass.h"

int main() {

	Window window;
	window.Create(500, 500, "Graphics Playground");

	LOG::LogLine("Starting Graphics");
	Graphics gfx;
	gfx.StartUp();
	gfx.AddWindow(&window);
	gfx.Initalize();

	RenderPass test;
	test.Create();

    while (!window.ShouldClose()) {
        window.Update();
    }

	return 0;
}