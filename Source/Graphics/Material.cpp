#include "Material.h"

#include "Graphics.h"
#include "Image.h"
#include "Buffer.h"

#include "PlatformDebug.h"

void MaterialBase::AddBinding(const uint32_t aBinding, const uint32_t aCount, const VkDescriptorType aType, const VkShaderStageFlags aStages) {
	VkDescriptorSetLayoutBinding binding = {};
	// binding.binding = 0;
	// binding.descriptorCount = 1;
	// binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding.binding			= aBinding;
	binding.descriptorCount = aCount;
	binding.descriptorType	= aType;
	binding.stageFlags		= aStages;
	mBindings.push_back(binding);
}

void MaterialBase::Create(const char* aName /* = 0*/) {
	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType						   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount					   = mBindings.size();
	createInfo.pBindings					   = mBindings.data();
	vkCreateDescriptorSetLayout(gGraphics->GetVkDevice(), &createInfo, GetAllocationCallback(), &mLayout);
	SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, mLayout, aName ? aName : "Unnamed Descriptor Set Layout");
}

void MaterialBase::Destroy() {
	vkDestroyDescriptorSetLayout(gGraphics->GetVkDevice(), mLayout, GetAllocationCallback());
}

std::vector<Material> MaterialBase::AllocateMaterials() {
	std::vector<Material> materials;

	Material material;
	material.Create(this, "Allocated Material");
	materials.push_back(material);

	return materials;
}

void Material::Create(const MaterialBase* aBase, const char* aName /* = 0*/) {
	mBase = aBase;

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType					   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool			   = gGraphics->GetDesciptorPool();
	alloc_info.descriptorSetCount		   = 1;
	alloc_info.pSetLayouts				   = &mBase->mLayout;
	VkResult err						   = vkAllocateDescriptorSets(gGraphics->GetVkDevice(), &alloc_info, &mSet);
	if(err == VK_ERROR_OUT_OF_POOL_MEMORY){
		LOGGER::Formated("Failed to allocate {0}, not enough pool memory", aName);
		ASSERT(false);
		return;
	}
	SetVkName(VK_OBJECT_TYPE_DESCRIPTOR_SET, mSet, aName ? aName : "Unnamed Descriptor Set");
}

void Material::SetImages(const Image& aImage, const uint8_t aBinding, const uint8_t aIndex) const {

	const VkDescriptorSetLayoutBinding& binding = mBase->mBindings[aBinding];

	VkDescriptorImageInfo* descriptors = (VkDescriptorImageInfo*)alloca(sizeof(VkDescriptorImageInfo) * binding.descriptorCount);
	for(int i = 0; i < binding.descriptorCount; i++) {
		descriptors[i].sampler	   = gGraphics->GetDefaultSampler();
		descriptors[i].imageView   = aImage.GetImageView();
		descriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkWriteDescriptorSet write_desc = {};
	write_desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.dstSet				= mSet;
	write_desc.dstBinding			= aBinding;
	write_desc.descriptorCount		= binding.descriptorCount;

	write_desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_desc.pImageInfo	  = descriptors;

	vkUpdateDescriptorSets(gGraphics->GetVkDevice(), 1, &write_desc, 0, NULL);
}

void Material::SetBuffers(const Buffer& aBuffer, const uint8_t aBinding, const uint8_t aIndex) const {

	const VkDescriptorSetLayoutBinding& binding = mBase->mBindings[aBinding];

	VkDescriptorBufferInfo* descriptors = (VkDescriptorBufferInfo*)alloca(sizeof(VkDescriptorBufferInfo) * binding.descriptorCount);
	for(int i = 0; i < binding.descriptorCount; i++) {
		descriptors[i].buffer = aBuffer.GetBuffer();
		descriptors[i].offset = 0;
		descriptors[i].range  = VK_WHOLE_SIZE;
	}

	VkWriteDescriptorSet write_desc = {};
	write_desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.dstSet				= mSet;
	write_desc.dstBinding			= aBinding;
	write_desc.descriptorCount		= binding.descriptorCount;

	write_desc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_desc.pBufferInfo	  = descriptors;

	vkUpdateDescriptorSets(gGraphics->GetVkDevice(), 1, &write_desc, 0, NULL);
}
