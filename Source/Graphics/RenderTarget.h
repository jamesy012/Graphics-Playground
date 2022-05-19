#pragma once

#include "Image.h"
#include "Framebuffer.h"

#include "Helpers.h"

class RenderPass;

class RenderTarget {
public:
    void Create(const ImageSize aImageSize, const RenderPass& aRenderPass);

    const Framebuffer& GetFb() const {
        return mFramebuffer;
    }
    const Image& GetImage() const {
        return mImage;
    }
private:
    Image mImage;
    Framebuffer mFramebuffer;

};