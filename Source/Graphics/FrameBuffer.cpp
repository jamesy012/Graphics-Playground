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

    info.height = aImage.GetImageSize().height;
    info.width = aImage.GetImageSize().width;
    info.layers = 1;

    info.flags = 0;

    vkCreateFramebuffer(gGraphics->GetVkDevice(), &info, GetAllocationCallback(), &mFramebuffer);
}