#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <PlatformDebug.h>

class Framebuffer;

class RenderPass {
public:
	void AddColorAttachment(VkFormat aFormat, VkAttachmentLoadOp aLoad) {
		//layout assumed to be VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		mAttachments.push_back(VkAttachmentDescription());
		mColorReferences.push_back(VkAttachmentReference());
		VkAttachmentDescription& des = mAttachments.back();
		des.finalLayout = des.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		des.format							= aFormat;
		des.loadOp							= aLoad;
		des.samples							= VK_SAMPLE_COUNT_1_BIT;
		des.storeOp							= VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference& ref = mColorReferences.back();
		ref.attachment			   = mAttachments.size() - 1;
		ref.layout				   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	void AddDepthAttachment(VkFormat aFormat, VkAttachmentLoadOp aLoad) {
		ASSERT(mDepthReference.layout == 0);
		//layout assumed to be VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		mAttachments.push_back(VkAttachmentDescription());
		VkAttachmentDescription& des = mAttachments.back();
		des.finalLayout = des.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		des.format							= aFormat;
		des.loadOp							= aLoad;
		des.samples							= VK_SAMPLE_COUNT_1_BIT;
		des.storeOp							= VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference& ref = mDepthReference;
		ref.attachment			   = mAttachments.size() - 1;
		ref.layout				   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	void SetMultiViewSupport(bool aMultiView) {
		mIsMultiView = aMultiView;
	}

	void Create(const char* aName = 0);
	void Destroy();

	void Begin(VkCommandBuffer aBuffer, const Framebuffer& aFramebuffer) const;
	void End(VkCommandBuffer aBuffer) const;

	const VkRenderPass GetRenderPass() const {
		return mRenderPass;
	}

	void SetClearColors(std::vector<VkClearValue> aClearColors);

private:
	std::vector<VkClearValue> mClearColors				= {};
	VkRenderPass mRenderPass							= VK_NULL_HANDLE;
	std::vector<VkAttachmentDescription> mAttachments	= {};
	std::vector<VkAttachmentReference> mColorReferences = {};
	VkAttachmentReference mDepthReference				= {};

	bool mIsMultiView = false;
};