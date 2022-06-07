#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Image;
class MaterialBase;

class Material {
public:
	void Create(const MaterialBase* aBase, const char* aName = 0);

	void SetImage(const Image& aImage) const;

	const VkDescriptorSet* GetSet() const {
		return &mSet;
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

    std::vector<Material> AllocateMaterials();

	VkDescriptorSetLayout mLayout = VK_NULL_HANDLE;

private:
	std::vector<VkDescriptorSetLayoutBinding> mBindings = {};
};