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
#include <math.h>
void RenderPass::Begin(VkCommandBuffer aBuffer, Framebuffer& aFramebuffer) {

    std::vector<VkClearValue> clearColors(1);
	clearColors[0].color = { { ((sin((300+gGraphics->GetFrameCount()/500.0f))) + 1) / 2.0f, ((sin((12+gGraphics->GetFrameCount()/280.0f))) + 1) / 2.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo renderBegin{};
    renderBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBegin.renderPass = mRenderPass;
    renderBegin.clearValueCount = static_cast<uint32_t>(clearColors.size());
    renderBegin.pClearValues = clearColors.data();


    renderBegin.framebuffer = aFramebuffer.GetFramebuffer();
    renderBegin.renderArea.extent = aFramebuffer.GetSize();

    vkCmdBeginRenderPass(aBuffer, &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
    VkRect2D scissor = { {}, gGraphics->GetMainSwapchain()->GetSize() };
    VkViewport viewport = {0.0f,0.0f,(float)scissor.extent.width, (float)scissor.extent.height, 0.0f, 1.0f};
    vkCmdSetScissor(aBuffer, 0, 1, &scissor);
    vkCmdSetViewport(aBuffer, 0, 1, &viewport);
}

void RenderPass::End(VkCommandBuffer aBuffer) {
    vkCmdEndRenderPass(aBuffer);
}