#include "RenderTarget.h"

#include "Graphics.h"

void RenderTarget::Create(const ImageSize aImageSize, const RenderPass& aRenderPass) {
	assert(false);
	//mImage.CreateVkImage(gGraphics->GetMainFormat(), aImageSize);
	//mFramebuffer.AddImage(mImage);
	mFramebuffer.Create(aRenderPass);
}