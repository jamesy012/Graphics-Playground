#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class Image;

class MaterialBase {
public:
	MaterialBase();

	void AddBinding(const uint32_t aBinding, const uint32_t aCount, const VkDescriptorType aType, const VkShaderStageFlags aStages);

	void Create(const char* aName = 0);

	void Destroy();

	VkDescriptorSetLayout mLayout = VK_NULL_HANDLE;

private:
	std::vector<VkDescriptorSetLayoutBinding> mBindings;
};

class Material {
public:
	Material();
	void Create(const MaterialBase* aBase, const char* aName = 0);

	void SetImage(const Image& aImage) const;

	const VkDescriptorSet* GetSet() const {
		return &mSet;
	}

private:
	const MaterialBase* mBase;
	VkDescriptorSet mSet = VK_NULL_HANDLE;
};