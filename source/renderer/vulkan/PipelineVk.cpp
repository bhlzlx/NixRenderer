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
#include <ks/io/io.h>
#include <mutex>
#include <cassert>

namespace nix {

	VkPipelineCache PipelineVk::GeneralPipelineCache = VK_NULL_HANDLE;

	void PipelineVk::begin()
	{
		auto cmd = m_context->getGraphicsQueue()->commandBuffer();
		m_commandBuffer = *cmd;

		vkCmdSetStencilReference(m_commandBuffer, VK_STENCIL_FRONT_AND_BACK, m_stencilReference);
		vkCmdSetBlendConstants(m_commandBuffer, m_blendConstants);
		vkCmdSetDepthBias(m_commandBuffer, m_constantBias, 0, m_slopeScaleBias);
		vkCmdSetViewport(m_commandBuffer, 0, 1, &m_viewport);
		vkCmdSetScissor(m_commandBuffer, 0, 1, &m_scissor);
		//
	}

	void PipelineVk::end()
	{
		m_commandBuffer = VK_NULL_HANDLE;
	}

	void PipelineVk::setViewport(const Viewport& _viewport)
	{
		m_viewport.width = _viewport.width;
		m_viewport.height = _viewport.height;
		m_viewport.x = _viewport.x;
		m_viewport.y = _viewport.y;
		m_viewport.maxDepth = _viewport.zFar;
		m_viewport.minDepth = _viewport.zNear;
	}

	void PipelineVk::setScissor(const Scissor& _scissor)
	{
		m_scissor.extent.width = _scissor.size.width;
		m_scissor.extent.height = _scissor.size.height;
		m_scissor.offset.x = _scissor.origin.x;
		m_scissor.offset.y = _scissor.origin.y;
	}

	void PipelineVk::bindVertexBuffer(uint32_t _index, IBuffer* _vertexBuffer, uint32_t _offset) {
		VkBuffer buffer = VK_NULL_HANDLE;
		if (_vertexBuffer->getType() == SVBO) {
			buffer = ((StableVertexBuffer*)_vertexBuffer)->m_buffer;
		}
		else if(_vertexBuffer->getType() == TVBO) {
			buffer = ((TransientVBO*)_vertexBuffer)->getBuffer();
		}
		else {
			assert(false && "buffer must be a vertex buffer!");
			return;
		}
		VkDeviceSize offset = _offset;
		vkCmdBindVertexBuffers(m_commandBuffer, _index, 1, &buffer, &offset);
	}

	void PipelineVk::draw(TopologyMode _pmode, uint32_t _offset, uint32_t _vertexCount) {
		auto pipeline = requestPipelineObject(_pmode);
		// bind pipeline
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// draw call
		vkCmdDraw(m_commandBuffer, _vertexCount, 1, _offset, 0);
	}

	void PipelineVk::drawIndexed(TopologyMode _pmode, IBuffer* _indexBuffer, uint32_t _indexOffset, uint32_t _indexCount)
	{
		auto pipeline = requestPipelineObject(_pmode);
		vkCmdBindIndexBuffer(m_commandBuffer, ((IndexBuffer*)_indexBuffer)->m_buffer, _indexOffset, VK_INDEX_TYPE_UINT16);
		// bind pipeline
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// draw call
		vkCmdDrawIndexed(m_commandBuffer, _indexCount, 1, _indexOffset, 0, 0);
	}

	void PipelineVk::drawInstanced(TopologyMode _pmode, uint32_t _offset, uint32_t _vertexCount, uint32_t _instanceCount) {
		auto pipeline = requestPipelineObject(_pmode);
		// bind pipeline
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// draw call
		vkCmdDraw(m_commandBuffer, _vertexCount, _instanceCount, 0, 0);
	}
	void PipelineVk::drawInstanced(TopologyMode _pmode, IBuffer* _indexBuffer, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _instanceCount) {
		auto pipeline = requestPipelineObject(_pmode);
		vkCmdBindIndexBuffer(m_commandBuffer, ((IndexBuffer*)_indexBuffer)->m_buffer, _indexOffset, VK_INDEX_TYPE_UINT16);
		// bind pipeline
		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// draw call
		vkCmdDrawIndexed(m_commandBuffer, _indexCount, 1, _indexOffset, 0, 0 );
	}

	void PipelineVk::setPolygonOffset(float _constantBias, float _slopeScaleBias)
	{
		m_constantBias = _constantBias;
		m_slopeScaleBias = _slopeScaleBias;
	}

	nix::IArgument* PipelineVk::createArgument(const ArgumentDescription& _desc)
	{
		std::vector<uint32_t> vecSetID;
		for (uint32_t i = 0; i < _desc.samplerCount; ++i) {
			uint32_t set;
			uint32_t binding;
			bool rst = m_vertexModule->GetSampler(_desc.samplerNames[i], set, binding);
			if (!rst) {
				rst = m_fragmentModule->GetSampler(_desc.samplerNames[i], set, binding);
			}
			if (!rst)// do not exists!
				return nullptr;
			vecSetID.push_back(set);
		}
		for (uint32_t i = 0; i < _desc.uboCount; ++i) {
			uint32_t set;
			uint32_t binding;
			uint32_t offset;
			bool rst = m_vertexModule->GetUniformMember( _desc.uboNames[i], set, binding, offset );
			if (!rst) {
				rst = m_fragmentModule->GetUniformMember(_desc.uboNames[i], set, binding, offset);
			}
			if (!rst) {
				// do not exists!
				assert(false);
				return nullptr;
			}
			vecSetID.push_back(set);
		}
		if (vecSetID.size() > 1) {
			auto it = vecSetID.begin();
			++it;
			while (it != vecSetID.end()) {
				if (*it != vecSetID[0]) {
					// descriptor set not the same!
					assert(false);
					return nullptr;
				}
				++it;
			}
		}
		return createArgument( vecSetID[0] );
	}

	nix::IArgument* PipelineVk::createArgument(uint32_t _setId)
	{
		auto set = m_context->getDescriptorSetPool().alloc(this, _setId);
		if (!set) {
			assert(false);
		}
		auto drawable = new ArgumentVk(set);
		drawable->m_pipeline = this;
		return drawable;
	}

	void PipelineVk::setShaderCache(const void* _data, size_t _size, size_t _offset) {
#ifndef NDEBUG
		assert( _size + _offset <= m_context->getDriver()->getPhysicalDeviceProperties().limits.maxPushConstantsSize );
#endif
		auto cmd = m_context->getGraphicsQueue()->commandBuffer();
		vkCmdPushConstants(cmd->operator const VkCommandBuffer &(), m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, _offset, _size, _data);
	}

	bool PipelineVk::getVertexShaderCacheMember(const std::string _name, size_t& size_, size_t& offset_) {
		return m_vertexModule->GetConstantsMember(_name, size_, offset_);
	}
	bool PipelineVk::getFragmentShaderCacheMember(const std::string _name, size_t& size_, size_t& offset_) {
		return m_fragmentModule->GetConstantsMember(_name, size_, offset_);
	}

	const nix::PipelineDescription& PipelineVk::getDescription()
	{
		return m_desc;
	}

	void PipelineVk::release()
	{
		auto deletor = GetDeferredDeletor();
		deletor.destroyResource(this);
	}

	PipelineVk::~PipelineVk()
	{
		auto device = m_context->getDevice();
		for (auto & pipeline : m_pipelines)
		{
			if (pipeline) {
				vkDestroyPipeline(device, pipeline, nullptr);
			}
		}
		vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
		for ( auto& setLayout : m_vecDescriptorSetLayout) {
			vkDestroyDescriptorSetLayout(device, setLayout.layout, nullptr);
		}
	}

	VkPipelineLayout PipelineVk::getPipelineLayout() const
	{
		return m_pipelineLayout;
	}

	VkPipelineLayout PipelineVk::CreateRenderPipelineLayout(const std::vector<ArgumentLayout>& _descriptorSetLayouts, uint32_t _maxPushConstantSize)
	{
		std::vector<VkDescriptorSetLayout> layouts;
		for (auto& d : _descriptorSetLayouts) {
			layouts.push_back(d.layout);
		}
		VkPushConstantRange constantsRange = {};
		{
			constantsRange.offset = 0;
			constantsRange.size = _maxPushConstantSize;
			constantsRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		VkPipelineLayoutCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			info.pSetLayouts = layouts.data();
			info.setLayoutCount = static_cast<uint32_t>(layouts.size());
			info.pushConstantRangeCount = 1;
			info.pPushConstantRanges = &constantsRange;
		}
		VkPipelineLayout pipelineLayout;
		vkCreatePipelineLayout(m_context->getDevice(), &info, nullptr, &pipelineLayout);
		return pipelineLayout;
	}

	IPipeline* ContextVk::createPipeline(const PipelineDescription& _desc) {
		auto device = m_context->getDevice();

		bool rst = false;
		TextReader vertReader, fragReader;
		rst = vertReader.openFile( m_archieve, std::string(_desc.vertexShader));
		rst = fragReader.openFile( m_archieve, std::string(_desc.fragmentShader));
		//auto vertShader = createShaderModule(vertReader.getText(), nix::ShaderModuleType::VertexShader);
		//auto fragShader = createShaderModule(fragReader.getText(), nix::ShaderModuleType::FragmentShader);
		auto vertShader = m_context->createShaderModule(vertReader.getText(), "main", VK_SHADER_STAGE_VERTEX_BIT);
		if (!vertShader)
			return nullptr;
		ShaderModuleVk* fragShader = nullptr;
		if ( _desc.fragmentShader)
			fragShader = m_context->createShaderModule(fragReader.getText(), "main", VK_SHADER_STAGE_FRAGMENT_BIT);
		//
		auto vecSetLayouts = ShaderModuleVk::CreateDescriptorSetLayout(vertShader, fragShader);
		auto pipelineLayout = PipelineVk::CreateRenderPipelineLayout(vecSetLayouts);

		auto renderPass = RenderPassVk::RequestRenderPassObject(_desc.renderPassDescription);
		//
		PipelineVk* pipeline = new PipelineVk();
		pipeline->m_desc = _desc;
		pipeline->m_vertexModule = vertShader;
		pipeline->m_fragmentModule = fragShader;
		pipeline->m_vecDescriptorSetLayout = vecSetLayouts;
		pipeline->m_pipelineLayout = pipelineLayout;
		pipeline->m_renderPass = renderPass;
		memset(pipeline->m_pipelines, 0, sizeof(pipeline->m_pipelines));
		//
		return pipeline;
	}

	bool PipelineVk::getUniformMember(const char * _name, uint32_t _setId, uint32_t & index_, uint32_t & offset_)
	{
		ShaderModuleVk* shaderModules[2] = { m_vertexModule, m_fragmentModule };
		index_ = 0;
		for (auto& shaderModule : shaderModules)
		{
			if (!shaderModule)
				break;
			auto& sets = shaderModule->GetDescriptorSetStructure();
			for (auto& set : sets)
			{
				if (set.id == _setId)
				{
					for (auto& unif : set.uniforms)
					{
						for (auto& member : unif.members)
						{
							if (member.name == _name)
							{
								offset_ = static_cast<uint32_t>(member.offset);
								return true;
							}
						}
						++index_;
					}
				}
			}
		}
		return false;
	}

	bool PipelineVk::getSampler(const char * _name, uint32_t _setId, uint32_t& binding_)
	{
		ShaderModuleVk* shaderModules[2] = { m_vertexModule, m_fragmentModule };
		binding_ = 0;
		for (auto& shaderModule : shaderModules)
		{
			if (!shaderModule)
				break;
			auto& sets = shaderModule->GetDescriptorSetStructure();
			for (auto& set : sets)
			{
				if (set.id == _setId)
				{
					for (auto& sampler : set.samplers)
					{
						if (sampler.name == _name)
						{
							binding_ = static_cast<uint32_t>(sampler.binding);
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	void PipelineVk::getUniformBindingPoints(size_t _setId, std::vector<UniformBindingPoint>& _bindingPoints)
	{
		_bindingPoints.clear();
		auto& vert_sets = m_vertexModule->GetDescriptorSetStructure();
		for (auto& set : vert_sets)
		{
			if (set.id == _setId)
			{
				for (auto& uniform : set.uniforms)
				{
					_bindingPoints.push_back(uniform);
				}
			}
		}
		if (m_fragmentModule)
		{
			auto& frag_sets = m_fragmentModule->GetDescriptorSetStructure();
			for (auto& set : frag_sets)
			{
				if (set.id == _setId)
				{
					for (auto& uniform : set.uniforms)
					{
						_bindingPoints.push_back(uniform);
					}
				}
			}
		}
	}

	void PipelineVk::getSamplerBindingPoints(uint32_t _setId, std::vector<SamplerBindingPoint>& _bindingPoints)
	{
		_bindingPoints.clear();
		auto& vert_sets = m_vertexModule->GetDescriptorSetStructure();
		for (auto& set : vert_sets)
		{
			if (set.id == _setId)
			{
				for (auto& sampler : set.samplers)
				{
					_bindingPoints.push_back(sampler);
				}
			}
		}
		if (m_fragmentModule)
		{
			auto& frag_sets = m_fragmentModule->GetDescriptorSetStructure();
			for (auto& set : frag_sets)
			{
				if (set.id == _setId)
				{
					for (auto& sampler : set.samplers)
					{
						_bindingPoints.push_back(sampler);
					}
				}
			}
		}
	}

	/*
	uint32_t PipelineVk::GetSamplerCount()
	{
		uint32_t count = 0;
		if (m_vertexModule)
			count += m_vertexModule->GetSamplerCount();
		if (m_fragmentModule)
			count += m_fragmentModule->GetSamplerCount();
		return count;
	}

	uint32_t PipelineVk::GetUniformBlockCount()
	{
		uint32_t count = 0;
		if (m_vertexModule)
			count += m_vertexModule->GetUniformBlockCount();
		if (m_fragmentModule)
			count += m_fragmentModule->GetUniformBlockCount();
		return count;
	}*/

	VkPipeline PipelineVk::requestPipelineObject( TopologyMode _topologyMode )
	{
		if (m_pipelines[_topologyMode]) {
			return m_pipelines[_topologyMode];
		}

		VkPrimitiveTopology tm = KsTopologyToVk(_topologyMode);
		VkPolygonMode pm = KsTopolygyPolygonMode(_topologyMode);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
		pipelineCreateInfo.layout = m_pipelineLayout;
		// render pass this pipeline is attached to
		pipelineCreateInfo.renderPass = m_renderPass;
		// Construct the different states making up the pipeline
		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = tm;
		//
		VkPipelineRasterizationStateCreateInfo rasterizationState; {
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.pNext = nullptr;
			rasterizationState.flags = 0;
			rasterizationState.polygonMode = pm;
			rasterizationState.cullMode = GxCullModeToVk(m_desc.renderState.cullMode);
			rasterizationState.frontFace = GxFrontFaceToVk(m_desc.renderState.windingMode);
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
				colorAttachmentBlendState.blendEnable = m_desc.renderState.blendState.blendEnable;
				colorAttachmentBlendState.alphaBlendOp = KsBlendOpToVk(m_desc.renderState.blendState.blendOperation);
				colorAttachmentBlendState.colorBlendOp = KsBlendOpToVk(m_desc.renderState.blendState.blendOperation);
				colorAttachmentBlendState.dstAlphaBlendFactor = KsBlendFactorToVk(m_desc.renderState.blendState.blendDestination);
				colorAttachmentBlendState.dstColorBlendFactor = KsBlendFactorToVk(m_desc.renderState.blendState.blendDestination);
				colorAttachmentBlendState.srcAlphaBlendFactor = KsBlendFactorToVk(m_desc.renderState.blendState.blendSource);
				colorAttachmentBlendState.srcColorBlendFactor = KsBlendFactorToVk(m_desc.renderState.blendState.blendSource);
				colorAttachmentBlendState.colorWriteMask = m_desc.renderState.writeMask;
				//
				blendAttachmentState[1] = blendAttachmentState[0];
				blendAttachmentState[2] = blendAttachmentState[1];
				blendAttachmentState[3] = blendAttachmentState[2];
			}
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.pNext = nullptr;
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = m_desc.renderPassDescription.colorAttachmentCount;
		colorBlendState.pAttachments = blendAttachmentState;
		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overriden by the dynamic states (see below) ���ﶯ̬������
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissor using dynamic states
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
			depthStencilState.depthTestEnable = m_desc.renderState.depthState.depthTestEnable;
			depthStencilState.depthWriteEnable = m_desc.renderState.depthState.depthWriteEnable;
			depthStencilState.depthCompareOp = KsCompareOpToVk(m_desc.renderState.depthState.depthFunction);
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.back.failOp = KsStencilOpToVk(m_desc.renderState.stencilState.stencilFail);
			depthStencilState.back.depthFailOp = KsStencilOpToVk(m_desc.renderState.stencilState.stencilZFail);
			depthStencilState.back.passOp = KsStencilOpToVk(m_desc.renderState.stencilState.stencilPass);
			depthStencilState.back.writeMask = m_desc.renderState.stencilState.stencilMask;
			depthStencilState.back.compareMask = 0xff;
			depthStencilState.back.compareOp = KsCompareOpToVk(m_desc.renderState.stencilState.stencilFunction);
			depthStencilState.minDepthBounds = 0;
			depthStencilState.maxDepthBounds = 1.0;
			depthStencilState.stencilTestEnable = m_desc.renderState.stencilState.stencilEnable;
			depthStencilState.front = depthStencilState.back;
			depthStencilState.back.failOp = KsStencilOpToVk(m_desc.renderState.stencilState.stencilFailCCW);
			depthStencilState.back.depthFailOp = KsStencilOpToVk(m_desc.renderState.stencilState.stencilZFailCCW);
			depthStencilState.back.passOp = KsStencilOpToVk(m_desc.renderState.stencilState.stencilPassCCW);
		}
		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.pNext = nullptr;
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.pSampleMask = nullptr;

		std::array< VkVertexInputBindingDescription, 16 > vertexInputBindings;
		std::array<VkVertexInputAttributeDescription, 16 > vertexInputAttributs;
		for (uint32_t i = 0; i < m_desc.vertexLayout.vertexBufferCount; ++i )
		{
			vertexInputBindings[i].binding = i;
			vertexInputBindings[i].stride = m_desc.vertexLayout.vertexBuffers[i].stride;
			vertexInputBindings[i].inputRate = m_desc.vertexLayout.vertexBuffers[i].instanceMode ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
		}

		for (uint32_t i = 0; i < m_desc.vertexLayout.vertexAttributeCount; ++i)
		{
			vertexInputAttributs[i].binding = m_desc.vertexLayout.vertexAttributes[i].bufferIndex;
			vertexInputAttributs[i].location = i;
			vertexInputAttributs[i].format = KsVertexFormatToVK(m_desc.vertexLayout.vertexAttributes[i].type);
			vertexInputAttributs[i].offset = m_desc.vertexLayout.vertexAttributes[i].offset;
		}
		// Vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = m_desc.vertexLayout.vertexBufferCount;
		vertexInputState.pVertexBindingDescriptions = &vertexInputBindings[0];
		vertexInputState.vertexAttributeDescriptionCount = m_desc.vertexLayout.vertexAttributeCount;
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();
		// Shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
		// Vertex shader
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// Set pipeline stage for this shader
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		// Load binary SPIR-V shader
		shaderStages[0].module = (const VkShaderModule&)*m_vertexModule;
		// Main entry point for the shader
		shaderStages[0].pName = "main";

		assert(shaderStages[0].module != VK_NULL_HANDLE);

		// Fragment shader
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		// Set pipeline stage for this shader
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		// Load binary SPIR-V shader
		if (m_fragmentModule) {
			shaderStages[1].module = (const VkShaderModule&)*m_fragmentModule;
			// Main entry point for the shader
			shaderStages[1].pName = "main";
			//
			assert(shaderStages[1].module != VK_NULL_HANDLE);
		}
		// Set pipeline shader stage info
		if (!m_fragmentModule)
			pipelineCreateInfo.stageCount = 1;
		else
			pipelineCreateInfo.stageCount = 2;
		//
		pipelineCreateInfo.pStages = shaderStages.data();
		// Assign the pipeline states to the pipeline creation info structure
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.renderPass = m_renderPass;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		//
		if (GeneralPipelineCache == VK_NULL_HANDLE) {
			struct alignas(4) CacheMem {
				uint32_t headerSize = 16 + VK_UUID_SIZE;
				uint32_t headerVersion = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
				uint32_t vendorID = 0;
				uint32_t deviceID = 0;
				char     cacheID[VK_UUID_SIZE];
			} cacheMem;
			cacheMem.vendorID = m_context->getDeviceProperties().vendorID;
			cacheMem.deviceID = m_context->getDeviceProperties().deviceID;
			memcpy(cacheMem.cacheID, m_context->getDeviceProperties().pipelineCacheUUID, sizeof(VK_UUID_SIZE));

			VkPipelineCacheCreateInfo cacheCreateInfo = {}; {
				cacheCreateInfo.flags = 0;
				cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
				cacheCreateInfo.initialDataSize = 16 + VK_UUID_SIZE;
				cacheCreateInfo.pInitialData = &cacheMem;
				cacheCreateInfo.pNext = nullptr;
			}
			if (VK_SUCCESS != vkCreatePipelineCache(m_context->getDevice(), &cacheCreateInfo, nullptr, &GeneralPipelineCache)) {
				assert(false);
			}
		}
		vkCreateGraphicsPipelines(m_context->getDevice(), GeneralPipelineCache, 1, &pipelineCreateInfo, nullptr, &m_pipelines[_topologyMode]);
		//
		return m_pipelines[_topologyMode];
	}

	void PipelineVk::setStencilReference(uint32_t _stencilReference) {
		m_stencilReference = _stencilReference;
	}

	void PipelineVk::setBlendFactor(const float* _blendFactors) {
		memcpy(m_blendConstants, _blendFactors, sizeof(m_blendConstants));
	}
	// create sampler must be a thread safe function
	// engine may be create resources in a background thread
	std::map< SamplerState, VkSampler > SamplerMapVk;
	std::mutex SamplerMapMutex;
	VkSampler GetSampler(const SamplerState& _state)
	{
		// construct a `key`
		union {
			struct alignas(1) {
				uint8_t AddressU;
				uint8_t AddressV;
				uint8_t AddressW;
				uint8_t MinFilter;
				uint8_t MagFilter;
				uint8_t MipFilter;
				uint8_t TexCompareMode;
				uint8_t TexCompareFunc;
			} u;
			SamplerState k;
		} key = {
			static_cast<uint8_t>(_state.u),
			static_cast<uint8_t>(_state.v),
			static_cast<uint8_t>(_state.w),
			static_cast<uint8_t>(_state.min),
			static_cast<uint8_t>(_state.mag),
			static_cast<uint8_t>(_state.mip),
			static_cast<uint8_t>(_state.compareMode),
			static_cast<uint8_t>(_state.compareFunction)
		};
		SamplerMapMutex.lock();
		// find a sampler by `key`
		auto rst = SamplerMapVk.find(key.k);
		if (rst != SamplerMapVk.end()) {
			SamplerMapMutex.unlock();
			return rst->second;
		}
		// cannot find a `sampler`, create a new one
		auto sampler = m_context->createSampler(_state);
		SamplerMapVk[key.k] = sampler; // insert the sampler to the map
		SamplerMapMutex.unlock();
		return sampler;
	}
}