#include "RenderPass.h"

#include "Graphics.h"
#include "Swapchain.h"

#include "Framebuffer.h"

void RenderPass::Create() {

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = gGraphics->GetMainSwapchain()->GetColorFormat();
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;

    VkRenderPassCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create.attachmentCount = 1;
    create.pAttachments = &colorAttachment;
    create.subpassCount = 1;
    create.pSubpasses = &subpass;

    vkCreateRenderPass(gGraphics->GetVkDevice(), &create, GetAllocationCallback(), &mRenderPass);

}

void RenderPass::Begin(VkCommandBuffer aBuffer, Framebuffer& aFramebuffer) {

    std::vector<VkClearValue> clearColors(1);
    float clearCol[4] = {0,1,0,1};
    memcpy(clearColors[0].color.float32, clearCol, sizeof(float)*4);
    

    VkRenderPassBeginInfo renderBegin{};
    renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBegin.renderPass = mRenderPass;
    renderBegin.clearValueCount = static_cast<uint32_t>(clearColors.size());
    renderBegin.pClearValues = clearColors.data();


    renderBegin.framebuffer = aFramebuffer.GetFramebuffer();
    renderBegin.renderArea.extent = aFramebuffer.GetSize();

    vkCmdBeginRenderPass(aBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
    //VkViewport viewport = GetViewport();
    //VkRect2D scissor = { {}, GetSize() };
    //vkCmdSetViewport(aBuffer, 0, 1, &viewport);
    //vkCmdSetScissor(aBuffer, 0, 1, &scissor);
}

void RenderPass::End(VkCommandBuffer aBuffer) {
    vkCmdEndRenderPass(aBuffer);
}