#include "Framebuffer.h"

#include "Graphics.h"

#include "Image.h"
#include "RenderPass.h"

void Framebuffer::Create(const RenderPass& mRenderPassTemplate, const char* aName /*= 0*/) {
	const size_t numImages = mLinkedImages.size();

	std::vector<VkImageView> attachments(numImages);

	for(size_t i = 0; i < numImages; i++) {
		attachments[i] = mLinkedImages[i]->GetImageView();
	}

	VkFramebufferCreateInfo info = {};
	info.sType					 = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass				 = mRenderPassTemplate.GetRenderPass();

	info.attachmentCount = numImages;
	info.pAttachments	 = attachments.data();

	//todo check that the image sizes match
	info.height = mLinkedImages[0]->GetImageSize().mHeight;
	info.width	= mLinkedImages[0]->GetImageSize().mWidth;
	info.layers = 1;

	info.flags = 0;

	vkCreateFramebuffer(gGraphics->GetVkDevice(), &info, GetAllocationCallback(), &mFramebuffer);

	mSize.width	 = info.width;
	mSize.height = info.height;

	SetVkName(VK_OBJECT_TYPE_FRAMEBUFFER, mFramebuffer, aName ? aName : "Unnamed FrameBuffer");
}

void Framebuffer::Destroy() {
	vkDestroyFramebuffer(gGraphics->GetVkDevice(), mFramebuffer, GetAllocationCallback());
	mFramebuffer = VK_NULL_HANDLE;
}
