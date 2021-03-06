#include "MaterialManager.h"

#include <imgui/imgui.h>

#include "Graphics.h"
#include "Devices.h"

#include "Image.h" //temp??

void MaterialManager::Initalize() {
	mMaxTextures =
		gGraphics->GetMainDevice()->GetPrimaryDeviceData().mDeviceDescriptorIndexingProperties.maxPerStageDescriptorUpdateAfterBindSamplers;

	const uint32_t largeArrayTexCount = 10000;
	if(mMaxTextures < largeArrayTexCount) {
		mMode = PER_DRAW;
	}

	if(gGraphics->GetMainDevice()->GetPrimaryDeviceData().IsBindlessDescriptorsAvailable()) {

		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = 0;
		binding.descriptorCount = mMaxTextures;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = 1;
		createInfo.pBindings = &binding;
		createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layoutBindingFlags = {};

		layoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
		layoutBindingFlags.bindingCount = 1;
		layoutBindingFlags.pBindingFlags = &bindlessFlags;

		VulkanResursiveSetpNext(&createInfo, &layoutBindingFlags);

		vkCreateDescriptorSetLayout(gGraphics->GetVkDevice(), &createInfo, GetAllocationCallback(), &mTextureSetLayout);
		SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, mTextureSetLayout, "Global Texture Descriptor Set Layout");

		if(mMode == MODE::LARGE_ARRAY) {
			mTextureGroups[0].push_back(BindlessTextureGroup());
			BindlessTextureGroup* group = &mTextureGroups[0].back();

			group->Create(largeArrayTexCount);
		}
	}
}

void MaterialManager::Destroy() {
	for(int index = 0; index < 3; index++) {
		for(int i = 0; i < mTextureGroups[index].size(); i++) {
			vkFreeDescriptorSets(gGraphics->GetVkDevice(), gGraphics->GetDesciptorPool(), 1, &mTextureGroups[index][i].mTextureSet);
		}
		mTextureGroups[index].clear();
	}

	vkDestroyDescriptorSetLayout(gGraphics->GetVkDevice(), mTextureSetLayout, GetAllocationCallback());
}

void MaterialManager::NewFrame() {
	//per draw needs to clear out it's desccriptor sets when starting a new frame
	if(mMode == MODE::PER_DRAW) {
		const uint32_t index = gGraphics->GetCurrentImageIndex();
		//todo combine this into one call
		for(int i = 0; i < mTextureGroups[index].size(); i++) {
			vkFreeDescriptorSets(gGraphics->GetVkDevice(), gGraphics->GetDesciptorPool(), 1, &mTextureGroups[index][i].mTextureSet);
		}
		mTextureGroups[index].clear();
	}
}

void MaterialManager::ImGuiDraw() {
	if(ImGui::Begin("Material Manager")) {

		if(mMode == MODE::LARGE_ARRAY) {
			BindlessTextureGroup& group = mTextureGroups[0][0];
			char buf[32];
			sprintf(buf, "%d/%d", group.mGlobalImageIndex, group.mMaxImageCount);
			ImGui::ProgressBar(group.mGlobalImageIndex / (float)group.mMaxImageCount, ImVec2(-1.f, 0.f), buf);
		}
	}
	ImGui::End();
}

void MaterialManager::PrepareTexture(uint32_t& aTextureID, Image* aImage) {
	mRequestedTextures.push_back(RequestedTextureData(aTextureID, aImage));
}

const VkDescriptorSet* MaterialManager::FinializeTextureSet() {
	std::vector<RequestedTextureData> requestedTextures = mRequestedTextures;
	mRequestedTextures.clear();

	if(mMode == MODE::LARGE_ARRAY) {
		return LargeArrayFinializeTextureSet(requestedTextures);
	} else if(mMode == MODE::PER_DRAW) {
		return PerDrawFinializeTextureSet(requestedTextures);
	}
	ASSERT(false);

	return nullptr;
}

const VkDescriptorSet* MaterialManager::GetMainTextureSet() const {
	if(mMode == MODE::LARGE_ARRAY) {
		return &mTextureGroups[0][0].mTextureSet;
	}
	return nullptr;
}

uint32_t MaterialManager::AddTextureToGlobalSet(VkImageView aImageView) {
	if(mMode == MODE::LARGE_ARRAY) {
		return mTextureGroups[0][0].AddGlobalTexture(aImageView);
	}
	return -1;
}

void MaterialManager::BindlessTextureGroup::Create(uint32_t aCount) {
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = gGraphics->GetDesciptorPool();
	allocInfo.descriptorSetCount = 1;
	const VkDescriptorSetLayout layout = gGraphics->GetMaterialManager()->GetDescriptorLayout();
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT allocCountInfo = {};
	allocCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	allocCountInfo.descriptorSetCount = 1;
	allocCountInfo.pDescriptorCounts = &aCount;

	VulkanResursiveSetpNext(&allocInfo, &allocCountInfo);

	VkResult err = vkAllocateDescriptorSets(gGraphics->GetVkDevice(), &allocInfo, &mTextureSet);
	if(err == VK_ERROR_OUT_OF_POOL_MEMORY) {
		LOGGER::Formated("Failed to allocate Global texture set, not enough pool memory");
		ASSERT(false);
		return;
	}
	SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_SET, mTextureSet, "Global texture set");

	mMaxImageCount = aCount;
}

const int MaterialManager::BindlessTextureGroup::AddGlobalTexture(const VkImageView aImage) {

	VkDescriptorImageInfo descriptor = {};
	descriptor.sampler = gGraphics->GetDefaultSampler();
	descriptor.imageView = aImage;
	descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet write_desc = {};
	write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.dstSet = mTextureSet;
	write_desc.dstBinding = 0;
	write_desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_desc.pImageInfo = &descriptor;
	write_desc.descriptorCount = 1;
	write_desc.dstArrayElement = mGlobalImageIndex;

	vkUpdateDescriptorSets(gGraphics->GetVkDevice(), 1, &write_desc, 0, NULL);

	return mGlobalImageIndex++;
}

const VkDescriptorSet* MaterialManager::LargeArrayFinializeTextureSet(std::vector<RequestedTextureData>& aTextureData) {
	for(int i = 0; i < aTextureData.size(); i++) {
		if(aTextureData[i].mImage) {
			aTextureData[i].mTextureIDRef = aTextureData[i].mImage->GetGlobalIndex();
		} else {
			aTextureData[i].mTextureIDRef = 0; //error texture?
		}
	}
	return nullptr;
}
const VkDescriptorSet* MaterialManager::PerDrawFinializeTextureSet(std::vector<RequestedTextureData>& aTextureData) {
	mTextureGroups[gGraphics->GetCurrentImageIndex()].push_back(BindlessTextureGroup());
	BindlessTextureGroup* group = &mTextureGroups[gGraphics->GetCurrentImageIndex()].back();

	group->Create(aTextureData.size() + 1); //one for error tex

	int errorTexture = -1;

	for(int i = 0; i < aTextureData.size(); i++) {
		if(aTextureData[i].mImage) {
			aTextureData[i].mTextureIDRef = group->AddGlobalTexture(aTextureData[i].mImage->GetImageView());
		} else {
			if(errorTexture == -1) {
				errorTexture = group->AddGlobalTexture(CONSTANTS::IMAGE::gWhite->GetImageView());
			}
			aTextureData[i].mTextureIDRef = errorTexture;
		}
	}
	return &group->mTextureSet;
}
