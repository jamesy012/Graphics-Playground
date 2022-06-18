#include "Screenspace.h"

#include "Graphics.h"

void Screenspace::Create(const FileIO::Path& aFragmentPath, const char* aName /* = 0*/) {
	mPipeline.AddShader(std::string(WORK_DIR_REL) + "/Shaders/Screenspace/Screenspace.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	mPipeline.AddShader(std::string(WORK_DIR_REL) + aFragmentPath.mPath, VK_SHADER_STAGE_FRAGMENT_BIT);

	//this should not be making a renderpass?

	if(mAttachmentFormat == VK_FORMAT_UNDEFINED) {
#if defined(ENABLE_XR)
		mAttachmentFormat = gGraphics->GetXRSwapchainFormat();
#else
		mAttachmentFormat = gGraphics->GetSwapchainFormat();
#endif
	}
	mRenderPass.AddColorAttachment(mAttachmentFormat, VK_ATTACHMENT_LOAD_OP_LOAD);

	mRenderPass.Create(aName);

	VkPushConstantRange range = {};
	range.stageFlags		  = VK_SHADER_STAGE_VERTEX_BIT;
	range.size				  = sizeof(PushConstant);
	mPipeline.AddPushConstant(range);

	mPipeline.Create(mRenderPass.GetRenderPass(), aName ? aName : "Unnamed Screenspace");

	//duplicated data
	mIndexBuffer.Create(BufferType::INDEX, sizeof(uint16_t) * 3, "Screenspace Index Buffer");
	uint16_t* data = (uint16_t*)mIndexBuffer.Map();
	data[0]		   = 0;
	data[1]		   = 1;
	data[2]		   = 2;
	mIndexBuffer.UnMap();

	mMaterials = mPipeline.MakeMaterials();
}

void Screenspace::Render(const VkCommandBuffer aBuffer, const Framebuffer& aFramebuffer) const {

	mRenderPass.Begin(aBuffer, aFramebuffer);

	vkCmdBindIndexBuffer(aBuffer, mIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
	mPipeline.Begin(aBuffer);

	//assuming viewport size is correct

	for(int i = 0; i < mMaterials.size(); i++) {
		vkCmdBindDescriptorSets(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetPipeline().GetLayout(), 0, 1, mMaterials[i].GetSet(), 0, 0);
	}

	vkCmdDraw(aBuffer, 3, 1, 0, 0);
	mPipeline.End(aBuffer);

	mRenderPass.End(aBuffer);
}

void Screenspace::Destroy() {
	mIndexBuffer.Destroy();
	mPipeline.Destroy();
	mRenderPass.Destroy();
}