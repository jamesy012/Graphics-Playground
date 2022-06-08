#include <vulkan/vulkan.h>
#include <imgui.h>

#include "Graphics/Window.h"
#include "Graphics/Graphics.h"
#include "PlatformDebug.h"

#include "Graphics/Screenspace.h"
#include "Graphics/Image.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"

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
	ssImage.LoadImage(std::string(WORK_DIR_REL) + "WorkDir/Assets/quanternius/tree/MapleTree_Leaves.png", VK_FORMAT_R8G8B8A8_UNORM);
	Image ssImage2;
	ssImage2.LoadImage(std::string(WORK_DIR_REL) + "WorkDir/Assets/quanternius/tree/MapleTree_Bark.png", VK_FORMAT_R8G8B8A8_UNORM);
	
	MaterialBase ssTestBase;
	ssTestBase.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	ssTestBase.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	ssTestBase.Create();
	Screenspace ssTest;
	ssTest.AddMaterialBase(&ssTestBase);
	ssTest.Create("WorkDir/Shaders/Screenspace/Image2.frag.spv", "ScreenSpace ImageCopy");
	ssTest.GetMaterial(0).SetImages(ssImage, 0, 0);
	ssTest.GetMaterial(0).SetImages(ssImage2, 1, 0);

	Mesh meshTest;
	meshTest.LoadMesh(std::string(WORK_DIR_REL) + "WorkDir/Assets/quanternius/tree/MapleTree_5.fbx");

	while(!window.ShouldClose()) {
		window.Update();

		gfx.StartNewFrame();

		VkCommandBuffer buffer = gfx.GetCurrentGraphicsCommandBuffer();

		ssTest.Render(buffer, gfx.GetCurrentFrameBuffer());

		gfx.EndFrame();
	}

	ssTest.Destroy();
	ssTestBase.Destroy();
	ssImage.Destroy();
	ssImage2.Destroy();

	gfx.Destroy();

	window.Destroy();

	return 0;
}