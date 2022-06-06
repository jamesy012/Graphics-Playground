#pragma once

#include "Engine/FileIO.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "RenderPass.h"

class Screenspace {
public:
	void Create(const FileIO::Path& aFragmentPath, const char* aName = 0);

	void Render(const VkCommandBuffer aBuffer, const Framebuffer& aFramebuffer) const;

	void Destroy();

private:
	Pipeline mPipeline;
	Buffer mIndexBuffer; //todo - constant between screenspace shaders
    RenderPass mRenderPass;
};