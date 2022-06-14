#include "RenderPass.h"

#include "Graphics.h"
#include "Swapchain.h"

#include "Framebuffer.h"
#include "PlatformDebug.h"

void RenderPass::Create(const char* aName /*= 0*/) {

	//VkAttachmentDescription colorAttachment = {};
	//colorAttachment.format					= gGraphics->GetMainSwapchain()->GetColorFormat();
	//colorAttachment.initialLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//colorAttachment.finalLayout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//colorAttachment.loadOp					= VK_ATTACHMENT_LOAD_OP_LOAD; //= VK_ATTACHMENT_LOAD_OP_CLEAR;
	//colorAttachment.storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
	//colorAttachment.samples					= VK_SAMPLE_COUNT_1_BIT;

	//VkAttachmentReference colorReference {};
	//colorReference.attachment = 0;
	//colorReference.layout	  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.colorAttachmentCount = mColorReferences.size();
	subpass.pColorAttachments	 = mColorReferences.data();
	if(mDepthReference.layout != 0) {
		subpass.pDepthStencilAttachment = &mDepthReference;
	}

	VkRenderPassCreateInfo create {};
	create.sType		   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create.attachmentCount = mAttachments.size();
	create.pAttachments	   = mAttachments.data();
	create.subpassCount	   = 1;
	create.pSubpasses	   = &subpass;

	vkCreateRenderPass(gGraphics->GetVkDevice(), &create, GetAllocationCallback(), &mRenderPass);
	SetVkName(VK_OBJECT_TYPE_RENDER_PASS, mRenderPass, aName ? aName : "Unnamed RenderPass");
}

void RenderPass::Begin(VkCommandBuffer aBuffer, const Framebuffer& aFramebuffer) const {

	VkRenderPassBeginInfo renderBegin {};
	renderBegin.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderBegin.renderPass		= mRenderPass;
	renderBegin.clearValueCount = static_cast<uint32_t>(mClearColors.size());
	renderBegin.pClearValues	= mClearColors.data();

	renderBegin.framebuffer		  = aFramebuffer.GetFramebuffer();
	renderBegin.renderArea.extent = aFramebuffer.GetSize();

	vkCmdBeginRenderPass(aBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);

	//setup renderpass view
	VkRect2D scissor	= {{}, aFramebuffer.GetSize()};
	VkViewport viewport = {0.0f, 0.0f, (float)scissor.extent.width, (float)scissor.extent.height, 0.0f, 1.0f};
	vkCmdSetScissor(aBuffer, 0, 1, &scissor);
	vkCmdSetViewport(aBuffer, 0, 1, &viewport);
}

void RenderPass::End(VkCommandBuffer aBuffer) const {
	vkCmdEndRenderPass(aBuffer);
}

void RenderPass::Destroy() {
	vkDestroyRenderPass(gGraphics->GetVkDevice(), mRenderPass, GetAllocationCallback());
	mRenderPass = VK_NULL_HANDLE;
}

void RenderPass::SetClearColors(std::vector<VkClearValue> aClearColors) {
	mClearColors = aClearColors;
}