#include "StbImageLoader.h"

#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "PlatformDebug.h"
#include "Graphics/Image.h"

Job::Work StbImageLoader::GetWork(FileIO::Path aPath) {
	Job::Work work;
	AsyncLoadData* imageData = new AsyncLoadData();
	imageData->ptr = mImage;

	work.mUserData = imageData;
	work.mWorkPtr = [aPath](void* userData) {
		ZoneScoped;
		ZoneText(aPath.String().c_str(), aPath.String().size());
		AsyncLoadData* imageData = (AsyncLoadData*)userData;
		//todo
		imageData->mData = stbi_load(aPath.String().c_str(), &imageData->width, &imageData->height, &imageData->comp, STBI_rgb_alpha);
		ASSERT(imageData->mData != nullptr);
		//ASSERT(imageData->comp == 4);
		ZoneValue(imageData->width);
		ZoneValue(imageData->height);
	};
	//work.mFinishOnMainThread = true;
	work.mFinishPtr = [=](void* userData) {
		ZoneScoped;
		AsyncLoadData* imageData = (AsyncLoadData*)userData;
		imageData->ptr->CreateFromData(imageData->mData, imageData->ptr->mFormat, {imageData->width, imageData->height}, aPath.String().c_str());
		stbi_image_free(imageData->mData);
	};
	return work;
}