#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Image;
class Buffer;
class MaterialBase;

class Material {
public:
	void Create(const MaterialBase* aBase, const char* aName = 0);

	void SetImages(const Image& aImage, const uint8_t aBinding, const uint8_t aIndex) const;
	void SetBuffers(const Buffer& aBuffer, const uint8_t aBinding, const uint8_t aIndex) const;

	const VkDescriptorSet* GetSet() const {
		return &mSet;
	}

	const bool IsValid() const {
		return mBase != nullptr && mSet != VK_NULL_HANDLE;
	}

private:
	const MaterialBase* mBase = nullptr;
	VkDescriptorSet mSet	  = VK_NULL_HANDLE;
};

class MaterialBase {
public:
	void AddBinding(const uint32_t aBinding, const uint32_t aCount, const VkDescriptorType aType, const VkShaderStageFlags aStages);

	void Create(const char* aName = 0);
	void Destroy();

	std::vector<Material> MakeMaterials();

	VkDescriptorSetLayout mLayout = VK_NULL_HANDLE;

protected:
	friend Material;
	std::vector<VkDescriptorSetLayoutBinding> mBindings = {};
};