#pragma once

#include <vulkan/vulkan.h>

#include "LoaderBase.h"

class Image;

class StbImageLoader : public LoaderBase {
private:
	struct AsyncLoadData {
		unsigned char* mData;
		int width, height, comp;
		Image* ptr;
		VkFormat format;
	};

public:
	virtual Job::Work GetWork(FileIO::Path aPath) override;

private:
};
