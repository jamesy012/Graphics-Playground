#include "Screenspace.h"

void Screenspace::Create(const FileIO::Path& aFragmentPath, const char* aName /* = 0*/) {
	mPipeline.AddShader(std::string(WORK_DIR_REL) + "WorkDir/Shaders/Screenspace/Screenspace.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	mPipeline.AddShader(std::string(WORK_DIR_REL) + aFragmentPath.mPath, VK_SHADER_STAGE_FRAGMENT_BIT);

	mRenderPass.Create(aName);

	mPipeline.Create(mRenderPass.GetRenderPass(), aName ? aName : "Unnamed Screenspace");

    //duplicated data
	mIndexBuffer.Create(BufferType::INDEX, sizeof(uint16_t) * 3, "Screenspace Index Buffer");
	uint16_t* data = (uint16_t*)mIndexBuffer.Map();
	data[0]		   = 0;
	data[1]		   = 1;
	data[2]		   = 2;
	mIndexBuffer.UnMap();
}

void Screenspace::Render(const VkCommandBuffer aBuffer, const Framebuffer& aFramebuffer) const {

	mRenderPass.Begin(aBuffer, aFramebuffer);

	vkCmdBindIndexBuffer(aBuffer, mIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
	mPipeline.Begin(aBuffer);

	//assuming viewport size is correct

	vkCmdDraw(aBuffer, 3, 1, 0, 0);
	mPipeline.End(aBuffer);

	mRenderPass.End(aBuffer);
}

void Screenspace::Destroy() {
	mIndexBuffer.Destroy();
	mPipeline.Destroy();
	mRenderPass.Destroy();
}