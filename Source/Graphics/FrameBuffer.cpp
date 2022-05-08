#include "Framebuffer.h"

#include "Graphics.h"

#include "Image.h"
#include "RenderPass.h"

void Framebuffer::Create(const Image& aImage, const RenderPass& mRenderPassTemplate) {

    VkImageView attachments = aImage.GetImageView();

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = mRenderPassTemplate.GetRenderPass();

    info.attachmentCount = 1;
    info.pAttachments = &attachments;

    info.height = aImage.GetImageSize().mHeight;
    info.width = aImage.GetImageSize().mWidth;
    info.layers = 1;

    info.flags = 0;

    vkCreateFramebuffer(gGraphics->GetVkDevice(), &info, GetAllocationCallback(), &mFramebuffer);

    mSize.width = info.width;
    mSize.height = info.height;
}