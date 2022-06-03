#include "Material.h"

#include "Graphics.h"
#include "Image.h"

MaterialBase::MaterialBase() {}

void MaterialBase::AddBinding(const uint32_t aBinding, const uint32_t aCount,
							  const VkDescriptorType aType,
							  const VkShaderStageFlags aStages) {
	VkDescriptorSetLayoutBinding binding = {};
	// binding.binding = 0;
	// binding.descriptorCount = 1;
	// binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding.binding = aBinding;
	binding.descriptorCount = aCount;
	binding.descriptorType = aType;
	binding.stageFlags = aStages;
	mBindings.push_back(binding);
}

void MaterialBase::Create(const char *aName /* = 0*/) {
	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = mBindings.size();
	createInfo.pBindings = mBindings.data();
	vkCreateDescriptorSetLayout(gGraphics->GetVkDevice(), &createInfo,
								GetAllocationCallback(), &mLayout);
	SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, mLayout,
			  aName ? aName : "Unnamed Descriptor Set Layout");
}

void MaterialBase::Destroy(){
	vkDestroyDescriptorSetLayout(gGraphics->GetVkDevice(), mLayout, GetAllocationCallback());
}

Material::Material() {}

void Material::Create(const MaterialBase *aBase, const char *aName /* = 0*/) {
	mBase = aBase;

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = gGraphics->GetDesciptorPool();
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &mBase->mLayout;
	VkResult err =
		vkAllocateDescriptorSets(gGraphics->GetVkDevice(), &alloc_info, &mSet);
	SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_SET, mSet,
			  aName ? aName : "Unnamed Descriptor Set");
}

void Material::SetImage(const Image &aImage) const {

	VkDescriptorImageInfo desc_image[1] = {};
	desc_image[0].sampler = gGraphics->GetDefaultSampler();
	desc_image[0].imageView = aImage.GetImageView();
	desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet write_desc[1] = {};
	write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc[0].dstSet = mSet;
	write_desc[0].dstBinding = 0;
	write_desc[0].descriptorCount = 1;
	write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_desc[0].pImageInfo = desc_image;

	vkUpdateDescriptorSets(gGraphics->GetVkDevice(), 1, write_desc, 0, NULL);
}
