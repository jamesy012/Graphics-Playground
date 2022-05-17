#include "Pipeline.h"

#include <glm/glm.hpp>

#include "Graphics.h"

#include <imgui.h>

bool Pipeline::AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage) {
	// preload shaders??
	FileIO::File file = FileIO::LoadFile(aPath);

	VkPipelineShaderStageCreateInfo stageInfo = {};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = aStage;
	stageInfo.pName = "main";

	VkShaderModuleCreateInfo moduleInfo = {};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.pCode = (uint32_t*)file.mData;
	moduleInfo.codeSize = file.mSize;

	vkCreateShaderModule(gGraphics->GetVkDevice(), &moduleInfo,
		GetAllocationCallback(), &stageInfo.module);

	mShaders.push_back(stageInfo);

	return stageInfo.module != VK_NULL_HANDLE;
}

bool Pipeline::Create(VkRenderPass aPass) {
	VkPipelineCache pipelineCache;
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		vkCreatePipelineCache(gGraphics->GetVkDevice(), &pipelineCacheCreateInfo,
			GetAllocationCallback(), &pipelineCache);
	}

	//~~~ Vertex
	VkPipelineVertexInputStateCreateInfo vertexInfo = {};
	VkVertexInputBindingDescription vertexBinding = {};
	std::vector<VkVertexInputAttributeDescription> vertexAttribute = std::vector<VkVertexInputAttributeDescription>(3);
	{
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertexBinding.stride = sizeof(ImDrawVert);

		vertexAttribute[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttribute[0].offset = offsetof(ImDrawVert, pos);
		vertexAttribute[1].location = 1;
		vertexAttribute[1].format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttribute[1].offset = offsetof(ImDrawVert, uv);
		vertexAttribute[2].location = 2;
		vertexAttribute[2].format = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttribute[2].offset = offsetof(ImDrawVert, col);

		vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInfo.vertexBindingDescriptionCount = 1;
		vertexInfo.pVertexBindingDescriptions = &vertexBinding;
		vertexInfo.pVertexAttributeDescriptions = vertexAttribute.data();
		vertexInfo.vertexAttributeDescriptionCount = vertexAttribute.size();
	}

	//~~~ Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	{
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	//~~~ Viewport State
	VkPipelineViewportStateCreateInfo viewportInfo = {};
	{
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.scissorCount = 1;
		viewportInfo.viewportCount = 1;
	}


	//~~~ Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
	{
		rasterizationInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationInfo.rasterizerDiscardEnable = VK_TRUE;
		rasterizationInfo.lineWidth = 1.0f;
	}

	//~~~ Depth Stencil State
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	{
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = false;
		depthStencilState.depthWriteEnable = false;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	}

	//~~~ Color Blend State
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	VkPipelineColorBlendAttachmentState blendAttachmentState = {};
	{
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;
	}

	//~~~ Dynamic States
	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
	{
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
		dynamicStateInfo.pDynamicStates = dynamicStates;
	}

	//~~~ Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayout setLayout;
		VkDescriptorSetLayoutCreateInfo setInfo = {};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setInfo.pBindings = &binding;
		setInfo.bindingCount = 1;
		vkCreateDescriptorSetLayout(gGraphics->GetVkDevice(), &setInfo, GetAllocationCallback(), &setLayout);

		VkPushConstantRange pushRange = {};
		{
			pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			struct test {
				glm::vec2 mScale;
				glm::vec2 mUv;
			};
			pushRange.size = sizeof(test);
		}

		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pPushConstantRanges = &pushRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pSetLayouts = &setLayout;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = 0;
		pipelineLayoutInfo.setLayoutCount = 0;//temp to remove descriptor set

		vkCreatePipelineLayout(gGraphics->GetVkDevice(), &pipelineLayoutInfo,
			GetAllocationCallback(), &mPipelineLayout);
	}

	//~~~ VkGraphicsPipelineCreateInfo
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	createInfo.pStages = mShaders.data();
	createInfo.stageCount = mShaders.size();

	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &inputAssemblyInfo;
	createInfo.pTessellationState = 0;
	createInfo.pViewportState = &viewportInfo;
	createInfo.pRasterizationState = &rasterizationInfo;
	createInfo.pMultisampleState = 0;
	createInfo.pDepthStencilState = &depthStencilState;
	createInfo.pColorBlendState = &colorBlendState;
	createInfo.pDynamicState = &dynamicStateInfo;
	createInfo.layout = mPipelineLayout;
	createInfo.renderPass = aPass;

	vkCreateGraphicsPipelines(gGraphics->GetVkDevice(), pipelineCache, 1,
		&createInfo, GetAllocationCallback(), &mPipeline);

	return false;
}

void Pipeline::Begin(VkCommandBuffer aBuffer) {
	vkCmdBindPipeline(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
}

void Pipeline::End(VkCommandBuffer aBuffer) {

}