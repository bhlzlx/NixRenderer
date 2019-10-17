#include "PipelineVk.h"
#include "ContextVk.h"
#include "ShaderVk.h"
#include "RenderPassVk.h"
#include "ArgumentVk.h"
#include "TypemappingVk.h"
#include "QueueVk.h"
#include "DeferredDeletor.h"
#include "BufferVk.h"
#include "DriverVk.h"
#include <nix/io/archieve.h>
#include <mutex>
#include <cassert>
#include "MaterialVk.h"
#include <cassert>

namespace Nix {

	IPipeline* MaterialVk::createPipeline(const RenderPassDescription& _renderPass)
	{
		VkRenderPass renderPass = RenderPassVk::RequestCompatibleRenderPassObject(m_context, _renderPass);
		//
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		pipelineCreateInfo.layout = m_pipelineLayout;
		// render pass this pipeline is attached to
		pipelineCreateInfo.renderPass = renderPass;
		// Construct the different states making up the pipeline
		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = m_topology;
		//
		VkPipelineRasterizationStateCreateInfo rasterizationState; {
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.pNext = nullptr;
			rasterizationState.flags = 0;
			rasterizationState.polygonMode = m_pologonMode;
			rasterizationState.cullMode = NixCullModeToVk(m_description.renderState.cullMode);
			rasterizationState.frontFace = NixFrontFaceToVk(m_description.renderState.windingMode);
			rasterizationState.depthClampEnable = VK_FALSE;
			rasterizationState.rasterizerDiscardEnable = VK_FALSE;
			rasterizationState.depthBiasEnable = VK_TRUE;
			rasterizationState.lineWidth = 1.0f;
		}
		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used
		VkPipelineColorBlendAttachmentState blendAttachmentState[4];
		{
			VkPipelineColorBlendAttachmentState& colorAttachmentBlendState = blendAttachmentState[0];
			{
				colorAttachmentBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorAttachmentBlendState.blendEnable = m_description.renderState.blendState.enable;
				colorAttachmentBlendState.alphaBlendOp = NixBlendOpToVk(m_description.renderState.blendState.op);
				colorAttachmentBlendState.colorBlendOp = NixBlendOpToVk(m_description.renderState.blendState.op);
				colorAttachmentBlendState.dstAlphaBlendFactor = NixBlendFactorToVk(m_description.renderState.blendState.dstFactor);
				colorAttachmentBlendState.dstColorBlendFactor = NixBlendFactorToVk(m_description.renderState.blendState.dstFactor);
				colorAttachmentBlendState.srcAlphaBlendFactor = NixBlendFactorToVk(m_description.renderState.blendState.srcFactor);
				colorAttachmentBlendState.srcColorBlendFactor = NixBlendFactorToVk(m_description.renderState.blendState.srcFactor);
				colorAttachmentBlendState.colorWriteMask = m_description.renderState.writeMask;
				//
				blendAttachmentState[1] = blendAttachmentState[0];
				blendAttachmentState[2] = blendAttachmentState[1];
				blendAttachmentState[3] = blendAttachmentState[2];
			}
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.pNext = nullptr;
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = _renderPass.colorCount;
		colorBlendState.pAttachments = blendAttachmentState;
		// Viewport state sets the number of viewports and scissorRect used in this pipeline
		// Note: This is actually overriden by the dynamic states (see below) ���ﶯ̬������
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissorRect using dynamic states
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables.data();
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		VkPipelineDepthStencilStateCreateInfo depthStencilState; {
			depthStencilState.pNext = nullptr;
			depthStencilState.flags = 0;
			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.depthTestEnable = m_description.renderState.depthState.testable;
			depthStencilState.depthWriteEnable = m_description.renderState.depthState.writable;
			depthStencilState.depthCompareOp = NixCompareOpToVk(m_description.renderState.depthState.cmpFunc);
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.back.failOp = NixStencilOpToVk(m_description.renderState.stencilState.opFail);
			depthStencilState.back.depthFailOp = NixStencilOpToVk(m_description.renderState.stencilState.opZFail);
			depthStencilState.back.passOp = NixStencilOpToVk(m_description.renderState.stencilState.opPass);
			depthStencilState.back.writeMask = m_description.renderState.stencilState.mask;
			depthStencilState.back.compareMask = 0xff;
			depthStencilState.back.compareOp = NixCompareOpToVk(m_description.renderState.stencilState.cmpFunc);
			depthStencilState.minDepthBounds = 0;
			depthStencilState.maxDepthBounds = 1.0;
			depthStencilState.stencilTestEnable = m_description.renderState.stencilState.enable;
			depthStencilState.front = depthStencilState.back;
			depthStencilState.back.failOp = NixStencilOpToVk(m_description.renderState.stencilState.opFailCCW);
			depthStencilState.back.depthFailOp = NixStencilOpToVk(m_description.renderState.stencilState.opZFailCCW);
			depthStencilState.back.passOp = NixStencilOpToVk(m_description.renderState.stencilState.opPassCCW);
		}
		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.pNext = nullptr;
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.pSampleMask = nullptr;

		std::array< VkVertexInputBindingDescription, 16 > vertexInputBindings;
		std::array<VkVertexInputAttributeDescription, 16 > vertexInputAttributs;
		for (uint32_t i = 0; i < m_description.vertexLayout.bufferCount; ++i)
		{
			vertexInputBindings[i].binding = i;
			vertexInputBindings[i].stride = m_description.vertexLayout.buffers[i].stride;
			vertexInputBindings[i].inputRate = m_description.vertexLayout.buffers[i].instanceMode ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
		}

		for (uint32_t i = 0; i < m_description.vertexLayout.attributeCount; ++i)
		{
			vertexInputAttributs[i].binding = m_description.vertexLayout.attributes[i].bufferIndex;
			vertexInputAttributs[i].location = i;
			vertexInputAttributs[i].format = NixVertexFormatToVK(m_description.vertexLayout.attributes[i].type);
			vertexInputAttributs[i].offset = m_description.vertexLayout.attributes[i].offset;
		}
		// Vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = m_description.vertexLayout.bufferCount;
		vertexInputState.pVertexBindingDescriptions = &vertexInputBindings[0];
		vertexInputState.vertexAttributeDescriptionCount = m_description.vertexLayout.attributeCount;
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();
		// Shaders
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (uint32_t i = 0; i < ShaderModuleType::ShaderTypeCount; ++i) {
			if (m_shaderModules[i]) {
				VkPipelineShaderStageCreateInfo info = {};
				info.flags = 0;
				info.pName = "main";
				info.module = m_shaderModules[i];
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				info.stage = NixShaderStageToVk(ShaderModuleType(i));
				shaderStages.push_back(info);
			}
		}
		assert(shaderStages.size() >= 1);
		//
		pipelineCreateInfo.pStages = shaderStages.data();
		pipelineCreateInfo.stageCount = shaderStages.size();
		// Assign the pipeline states to the pipeline creation info structure
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		//
		VkPipeline ppl;
		auto pplCache = m_context->getPipelineCache();
		vkCreateGraphicsPipelines(m_context->getDevice(), pplCache, 1, &pipelineCreateInfo, nullptr, &ppl);

		PipelineVk* pipeline = new PipelineVk();
		pipeline->m_pipeline = ppl;
		pipeline->m_renderPass = renderPass;
		pipeline->m_context = m_context;

		return pipeline;
	}

	void PipelineVk::setDynamicalStates(VkCommandBuffer _commandBuffer)
	{
		//m_commandBuffer = _commandBuffer;
		vkCmdSetBlendConstants(_commandBuffer, m_blendConstants);
		// depthBiasClamp indicates whether depth bias clamping is supported.
		// If this feature is not enabled 
		// the depthBiasClamp member of the VkPipelineRasterizationStateCreateInfo structure must be set to 0.0 
		// unless the VK_DYNAMIC_STATE_DEPTH_BIAS dynamic state is enabled
		// and the depthBiasClamp parameter to vkCmdSetDepthBias must be set to 0.0
		vkCmdSetDepthBias(_commandBuffer, m_constantBias, 0.0f, m_slopeScaleBias);
		vkCmdSetStencilReference(_commandBuffer, VK_STENCIL_FRONT_AND_BACK, m_stencilReference);
		//vkCmdSetViewport(_commandBuffer, 0, 1, &m_viewport);
		//vkCmdSetScissor(_commandBuffer, 0, 1, &m_scissor);
	}

	void PipelineVk::setPolygonOffset(float _constantBias, float _slopeScaleBias)
	{
		m_constantBias = _constantBias;
		m_slopeScaleBias = _slopeScaleBias;
	}

	void PipelineVk::release()
	{
		auto deletor = GetDeferredDeletor();
		deletor.destroyResource(this);
	}

	PipelineVk::PipelineVk() :
		m_renderPass(VK_NULL_HANDLE),
		m_stencilReference(0),
		m_constantBias(0),
		m_slopeScaleBias(0)
	{
		m_blendConstants[0] = m_blendConstants[1] = m_blendConstants[2] = m_blendConstants[3] = 1.0f;
	}

	PipelineVk::~PipelineVk()
	{
		auto device = m_context->getDevice();
		if (m_pipeline) {
			vkDestroyPipeline(device, m_pipeline, nullptr);
		}
	}

	void PipelineVk::setStencilReference(uint32_t _stencilReference) {
		m_stencilReference = _stencilReference;
	}

	void PipelineVk::setBlendFactor(const float* _blendFactors) {
		memcpy(m_blendConstants, _blendFactors, sizeof(m_blendConstants));
	}
}
