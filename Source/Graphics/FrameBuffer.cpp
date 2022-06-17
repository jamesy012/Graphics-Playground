#include "Framebuffer.h"

#include "Graphics.h"

#include "Image.h"
#include "RenderPass.h"

void Framebuffer::Create(const RenderPass& mRenderPassTemplate, const char* aName /*= 0*/) {
	const size_t numImages = mLinkedImages.size();
	ASSERT(numImages != 0);
	std::vector<VkImageView> attachments(numImages);

	for(size_t i = 0; i < numImages; i++) {
		attachments[i] = mLinkedImages[i]->GetImageView();
	}
	//debug only
	if(mLinkedImages.size() != 0)
	{
		mSize = mLinkedImages[0]->GetImageSize();
		for(size_t i = 1; i < numImages; i++) {
			ASSERT(mLinkedImages[i]->GetImageSize() == mSize);
		}
	}

	VkFramebufferCreateInfo info = {};
	info.sType					 = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass				 = mRenderPassTemplate.GetRenderPass();

	info.attachmentCount = numImages;
	info.pAttachments	 = attachments.data();

	info.height = mSize.height;
	info.width	= mSize.width;
	info.layers = 1;

	info.flags = 0;

	vkCreateFramebuffer(gGraphics->GetVkDevice(), &info, GetAllocationCallback(), &mFramebuffer);

	SetVkName(VK_OBJECT_TYPE_FRAMEBUFFER, mFramebuffer, aName ? aName : "Unnamed FrameBuffer");
}

void Framebuffer::Destroy() {
	vkDestroyFramebuffer(gGraphics->GetVkDevice(), mFramebuffer, GetAllocationCallback());
	mFramebuffer = VK_NULL_HANDLE;
}
