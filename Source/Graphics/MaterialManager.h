#pragma once

#include <vector>

#include <vulkan/vulkan.h>

//todo replace with VkImageView
class Image;

//manages the bindless nature of the engine
//supports two modes of operation
//large array or per draw
//large array puts every image into an array giving a number to index into that (prefered)
//per draw creates a new array for each draw
//it picks the mode based on how big of an array the gpu supports

class MaterialManager {
	//todo look into using unordered map for O(1)? searches
	//todo improve per draw mode
    //could turn large array into pools
public:
	void Initalize();
	void Destroy();

	void NewFrame();

	VkDescriptorSetLayout GetDescriptorLayout() const {
		return mTextureSetLayout;
	}

    bool IsInLargeArrayMode() const {
        return mMode == MODE::LARGE_ARRAY;
    }

	//makes sure this view is ready for the next draw
	//sets aTextureID to reference where that view is
	void PrepareTexture(uint32_t& aTextureID, Image* aImage);
	//sets up the texture Ids from PrepareTexture
	//returns a descriptor set if using per draw mode
	const VkDescriptorSet* FinializeTextureSet();

	//valid during large array mode
	const VkDescriptorSet* GetMainTextureSet() const;

	//valid during large array mode
	//todo should possibly be removed
	//instead adding the view in when it's first used
	uint32_t AddTextureToGlobalSet(VkImageView aImageView);

private:
	//min of 16, max of int_max
	uint32_t mMaxTextures;
	enum MODE
	{
		LARGE_ARRAY,
		PER_DRAW
	};
	MODE mMode = MODE::LARGE_ARRAY;

	//common layout of group
	VkDescriptorSetLayout mTextureSetLayout;
	//
	struct BindlessTextureGroup {
        void Create(uint32_t aCount);
		const int AddGlobalTexture(const VkImageView aImage);

		VkDescriptorSet mTextureSet;
		std::vector<VkImageView> mTextures;
		int mGlobalImageIndex = 0;
	};
	//one for each frame
	std::vector<BindlessTextureGroup> mTextureGroups[3];
	struct RequestedTextureData {
		RequestedTextureData(uint32_t& aRef, Image* aImage) : mTextureIDRef(aRef), mImage(aImage) {}
		uint32_t& mTextureIDRef;
		Image* mImage;
	};
	std::vector<RequestedTextureData> mRequestedTextures;
	//unsigned int AddTexture(VkImageView aView);
	//VkDescriptorSet* FinializeTextureSet();

	//change this to being a class or function ptr array
	const VkDescriptorSet* LargeArrayFinializeTextureSet(std::vector<RequestedTextureData>& aTextureData);
	const VkDescriptorSet* PerDrawFinializeTextureSet(std::vector<RequestedTextureData>& aTextureData);
};