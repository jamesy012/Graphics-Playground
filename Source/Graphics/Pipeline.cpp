#include "Pipeline.h"

#include <glm/glm.hpp>

#include "Graphics.h"
#include "Material.h"
#include "PlatformDebug.h"

bool Pipeline::AddShader(FileIO::Path aPath, VkShaderStageFlagBits aStage) {
	// preload shaders??
	FileIO::File file = FileIO::LoadFile(aPath);

	VkPipelineShaderStageCreateInfo stageInfo = {};
	stageInfo.sType							  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage							  = aStage;
	stageInfo.pName							  = "main";

	VkShaderModuleCreateInfo moduleInfo = {};
	moduleInfo.sType					= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.pCode					= (uint32_t*)file.mData;
	moduleInfo.codeSize					= file.mSize;

	vkCreateShaderModule(gGraphics->GetVkDevice(), &moduleInfo, GetAllocationCallback(), &stageInfo.module);

	SetVkName(VK_OBJECT_TYPE_SHADER_MODULE, stageInfo.module, aPath);

	mShaders.push_back(stageInfo);

	return stageInfo.module != VK_NULL_HANDLE;
}

bool Pipeline::Create(VkRenderPass aPass, const char* aName /*= 0*/) {
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType					  = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		vkCreatePipelineCache(gGraphics->GetVkDevice(), &pipelineCacheCreateInfo, GetAllocationCallback(), &mPipelineCache);
		SetVkName(VK_OBJECT_TYPE_PIPELINE_CACHE, mPipelineCache, aName);
	}

	//~~~ Vertex
	VkPipelineVertexInputStateCreateInfo vertexInfo = {};
	{
		LOGGER::Log("-- Pipeline Vertex Input needs work\n");
		vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		if(vertexBinding.stride != 0) {
			vertexInfo.vertexBindingDescriptionCount = 1;
			vertexInfo.pVertexBindingDescriptions	 = &vertexBinding;
		}
		vertexInfo.pVertexAttributeDescriptions	   = vertexAttribute.data();
		vertexInfo.vertexAttributeDescriptionCount = vertexAttribute.size();
	}

	//~~~ Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	{
		inputAssemblyInfo.sType	   = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	//~~~ Viewport State
	VkPipelineViewportStateCreateInfo viewportInfo = {};
	{
		viewportInfo.sType		   = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.scissorCount  = 1;
		viewportInfo.viewportCount = 1;
	}

	//~~~ Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
	{
		rasterizationInfo.sType					  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.depthClampEnable		  = VK_FALSE;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.polygonMode			  = VK_POLYGON_MODE_FILL;
		rasterizationInfo.lineWidth				  = 1.0f;
		rasterizationInfo.cullMode				  = VK_CULL_MODE_NONE;
		rasterizationInfo.frontFace				  = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationInfo.depthBiasEnable		  = VK_TRUE;
		rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
		rasterizationInfo.depthBiasClamp		  = 0.0f; // Optional
		rasterizationInfo.depthBiasSlopeFactor	  = 0.0f; // Optional
	}

	//~~~ Depth Stencil State
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	{
		multisampleState.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.sampleShadingEnable   = VK_FALSE;
		multisampleState.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.minSampleShading	   = 1.0f; // Optional
		multisampleState.pSampleMask		   = nullptr; // Optional
		multisampleState.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampleState.alphaToOneEnable	   = VK_FALSE; // Optional
	}
	//~~~ Depth Stencil State
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	{
		depthStencilState.sType			   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable  = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
	}

	//~~~ Color Blend State
	VkPipelineColorBlendStateCreateInfo colorBlendState		 = {};
	VkPipelineColorBlendAttachmentState blendAttachmentState = {};
	{
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp		 = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp		 = VK_BLEND_OP_ADD;
		colorBlendState.sType					 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount			 = 1;
		colorBlendState.pAttachments			 = &blendAttachmentState;
		colorBlendState.logicOp					 = VK_LOGIC_OP_CLEAR;
		colorBlendState.logicOpEnable			 = VK_FALSE;
	}

	//~~~ Dynamic States
	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	const VkDynamicState dynamicStates[]			  = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	{
		dynamicStateInfo.sType			   = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
		dynamicStateInfo.pDynamicStates	   = dynamicStates;
	}

	//~~~ Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	{
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.pPushConstantRanges	  = mPushConstants.data();
		pipelineLayoutInfo.pushConstantRangeCount = mPushConstants.size();

		if(mMaterialBase != nullptr) {
			VkDescriptorSetLayout layout	  = mMaterialBase->mLayout;
			pipelineLayoutInfo.pSetLayouts	  = &layout;
			pipelineLayoutInfo.setLayoutCount = 1;
		}

		vkCreatePipelineLayout(gGraphics->GetVkDevice(), &pipelineLayoutInfo, GetAllocationCallback(), &mPipelineLayout);
		SetVkName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, mPipelineLayout, aName);
	}

	//~~~ VkGraphicsPipelineCreateInfo
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType						= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	createInfo.pStages	  = mShaders.data();
	createInfo.stageCount = mShaders.size();

	createInfo.pVertexInputState   = &vertexInfo;
	createInfo.pInputAssemblyState = &inputAssemblyInfo;
	createInfo.pTessellationState  = 0;
	createInfo.pViewportState	   = &viewportInfo;
	createInfo.pRasterizationState = &rasterizationInfo;
	createInfo.pMultisampleState   = &multisampleState;
	createInfo.pDepthStencilState  = &depthStencilState;
	createInfo.pColorBlendState	   = &colorBlendState;
	createInfo.pDynamicState	   = &dynamicStateInfo;
	createInfo.layout			   = mPipelineLayout;
	createInfo.renderPass		   = aPass;

	vkCreateGraphicsPipelines(gGraphics->GetVkDevice(), mPipelineCache, 1, &createInfo, GetAllocationCallback(), &mPipeline);
	SetVkName(VK_OBJECT_TYPE_PIPELINE, mPipeline, aName ? aName : "Unnamed Graphics Pipeline");

	for(int i = 0; i < mShaders.size(); i++) {
		vkDestroyShaderModule(gGraphics->GetVkDevice(), mShaders[i].module, GetAllocationCallback());
	}
	mShaders.clear();

	return true;
}

void Pipeline::Destroy() {
	vkDestroyPipelineCache(gGraphics->GetVkDevice(), mPipelineCache, GetAllocationCallback());
	vkDestroyPipelineLayout(gGraphics->GetVkDevice(), mPipelineLayout, GetAllocationCallback());
	vkDestroyPipeline(gGraphics->GetVkDevice(), mPipeline, GetAllocationCallback());
}

void Pipeline::Begin(VkCommandBuffer aBuffer) const {
	vkCmdBindPipeline(aBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
}

void Pipeline::End(VkCommandBuffer aBuffer) const {}

std::vector<Material> Pipeline::AllocateMaterials() const {
	return mMaterialBase->AllocateMaterials();
}