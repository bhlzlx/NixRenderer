#pragma once
#include "VkInc.h"
#include <NixRenderer.h>
#include <array>
#include <vector>
#include <map>
#include <VkShaderCompiler/VkShaderCompiler.h>

namespace Nix {
	//
	class ContextVk;
	class RenderableVk;
	class ArgumentVk;

	VkShaderStageFlagBits NixShaderStageToVk(ShaderModuleType _stage);

	using namespace Nix::spvcompiler;

	class ArgumentLayoutExt {
		using ArgumentUniformLayouts = std::vector<std::vector<GLSLStructMember>>;
		friend class MaterialVk;
		friend class ArgumentVk;
		friend class ArgumentPoolChunk;
		friend class ArgumentAllocator;
	private:
		uint32_t									m_setID;
		VkDescriptorSetLayout						m_setLayout;
		//
		std::vector<SubpassInput>					m_vecSubpassInput; // input attachment
		std::vector<SeparateSampler>				m_vecSampler; // samplers
		std::vector<SeparateImage>					m_vecSampledImage; // sampled image
		std::vector<StorageImage>					m_vecStorageImage; // read/write image
		std::vector<StorageImage>					m_vecTexelBuffer; // texel buffer
		std::vector<CombinedImageSampler>			m_vecCombinedImageSampler; // sampler & image
		//
		std::vector<UniformBuffer>					m_vecUniformBuffer; // uniform buffer
		std::vector<StorageBuffer>					m_vecStorageBuffer; // storage buffer

		std::vector<ShaderDescriptor>				m_vecShaderDescriptor;
		//
		ArgumentUniformLayouts						m_uniformLayouts; // 

		// VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER	   只读 Texel Buffer，
		// VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER     可读写 Texel Buffer
		// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER		   只读 Buffer(静态绑定)  (X)
		// VK_DESCRIPTOR_TYPE_STORAGE_BUFFER		   可读写 Buffer(静态绑定) (X)
		// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC   只读 Buffer
		// VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC   可读写 Buffer
		std::vector<VkDescriptorBufferInfo>			m_vecDescriptorBufferInfo;
		// VK_DESCRIPTOR_TYPE_SAMPLER 就是采样器 (X)
		// VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE 就是一个可采样的纹理 (X)
		// VK_DESCRIPTOR_TYPE_STORAGE_IMAGE 就是一个可写纹理
		// VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER 包含一个采样器和一个纹理（OpenGL style）
		// VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT 一个 input attachment
		std::vector<VkDescriptorImageInfo>			m_vecDescriptorImageInfo;
		std::vector<uint32_t>						m_dynamicOffsetIndexTable;
		// key : binding , value : offset index
		//std::map<uint32_t, uint32_t>				m_offsetTable;
		// offsets passed to the vkBindDescriptorSet
		//std::vector<uint32_t>						m_uniformIndices;
		//std::vector<uint32_t>						m_storageIndices;
		//std::vector<VkDeviceSize>					m_vecBufferBaseOffsets;
	private:
	public:
		ArgumentLayoutExt()
			: m_setID(-1)
			, m_setLayout(VK_NULL_HANDLE)
		{
		}
		// image descriptor
		const SeparateSampler* getSampler(const std::string& _name) const;
		const SeparateImage* getSampledImage(const std::string& _name) const;
		const StorageImage* getStorageImage(const std::string& _name) const;
		const CombinedImageSampler* getCombinedImageSampler(const std::string& _name) const;
		const SubpassInput* getSubpassInput(const std::string& _name) const;
		// buffer descriptor
		const StorageBuffer* getStorageBuffer(const std::string& _name) const;
		const UniformBuffer* getUniform(const std::string& _name, const std::vector<GLSLStructMember>*& _members) const;
		uint32_t getDynamicOffsetsIndex(uint32_t _binding) const {
			return m_dynamicOffsetIndexTable[_binding];
		}
		const std::vector<ShaderDescriptor>& getDescriptors() const {
			return m_vecShaderDescriptor;
		}
		uint32_t getSetID() const {
			return m_setID;
		}
		VkDescriptorSetLayout getSetLayout() const {
			return m_setLayout;
		}
		//
		//void updateUniformBufferOffsets(VkDeviceSize _newUniformOffset, std::vector<VkDeviceSize>& _offsets) {
		//	for (auto index : m_uniformIndices) {
		//		_offsets[index] = m_vecBufferBaseOffsets[index] + _newUniformOffset;
		//	}
		//}
		//void updateStorageBufferOffsets(VkDeviceSize _newStorageOffset, std::vector<VkDeviceSize>& _offsets) {
		//	for (auto index : m_storageIndices) {
		//		_offsets[index] = m_vecBufferBaseOffsets[index] + _newStorageOffset;
		//	}
		//}
	};

	class MaterialVk : public IMaterial {
		friend class ArgumentAllocator;
	private:
		MaterialDescription										m_description;
		ContextVk*												m_context;
		VkShaderModule											m_shaderModules[ShaderModuleType::ShaderTypeCount];
		uint32_t												m_descriptorSetLayoutCount;
		std::array<ArgumentLayoutExt, MaxArgumentCount>			m_argumentLayouts;
		//
		VkPipelineLayout										m_pipelineLayout;
		VkPrimitiveTopology										m_topology;
		VkPolygonMode											m_pologonMode;
		//
		uint32_t												m_constantsStageFlags;
	public:
		MaterialVk()
			: m_descriptorSetLayoutCount(0)
			, m_argumentLayouts()
			, m_pipelineLayout(VK_NULL_HANDLE)
		{
		}

		virtual IArgument* createArgument(uint32_t _index) override;
		virtual void destroyArgument(IArgument* _argument) override;

		virtual IRenderable* createRenderable() override;
		virtual void destroyRenderable(IRenderable* _renderable) override;

		virtual IPipeline* createPipeline(const RenderPassDescription& _renderPass) override;

		virtual void release() override;

		const ArgumentLayoutExt& getDescriptorSetLayout(uint32_t _setIndex) const {
			return m_argumentLayouts[_setIndex];
		}
		ContextVk* getContext() {
			return m_context;
		}
		const VkPipelineLayout& getPipelineLayout() const {
			return m_pipelineLayout;
		}
		const MaterialDescription& getDescription() const {
			return m_description;
		}
		uint32_t getConstantsStageFlags() const {
			return m_constantsStageFlags;
		}
		//
		static MaterialVk* CreateMaterial(ContextVk* _context, const MaterialDescription& _desc);
		static VkShaderModule CreateShaderModule(ContextVk* _context, const char * _shader, ShaderModuleType _type);
	};

}
