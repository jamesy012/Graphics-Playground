#include "RenderTarget.h"

#include "Graphics.h"

void RenderTarget::Create(const ImageSize aImageSize, const RenderPass& aRenderPass){
    mImage.CreateVkImage(gGraphics->GetMainFormat(), aImageSize);
    mFramebuffer.Create(mImage, aRenderPass);
}