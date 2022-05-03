#include "RenderPass.h"

#include "Graphics.h"
#include "SwapChain.h"

void RenderPass::Create() {

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = gGraphics->GetMainSwapChain()->GetColorFormat();
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkSubpassDescription subpass = {};
    subpass.inputAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;

    VkRenderPassCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create.attachmentCount = 1;
    create.pAttachments = &colorAttachment;
    create.subpassCount = 1;
    create.pSubpasses = &subpass;

    vkCreateRenderPass(gGraphics->GetVkDevice(), &create, GetAllocationCallback(), &mRenderPass);

}