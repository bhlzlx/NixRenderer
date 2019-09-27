#pragma once
#include "VkInc.h"
#include <NixRenderer.h>
#include <array>
#include <vector>
#include <VkShaderCompiler/VkShaderCompiler.h>

namespace Nix {
	//
	class ContextVk;
	class RenderableVk;
	class ArgumentVk;

	class ArgumentLayoutExt {
		using ArgumentUniformLayouts = std::vector<std::vector<UniformBlock::Member>>;
		friend class MaterialVk;
		friend class ArgumentVk;		
		friend ArgumentAllocator;
	private:
		uint32_t									m_setID;
		VkDescriptorSetLayout						m_setLayout;
		//
		std::vector<SubpassInput>					m_vecSubpassInput;
		std::vector<UniformBlock>					m_vecUniformBlock;
		std::vector<ShaderStorageBufferObject>		m_vecSSBO;
		//
		std::vector<TexelBufferObject>				m_vecTBO;
		std::vector<CombinedImageSampler>			m_vecSampler;
		//
		ArgumentUniformLayouts						m_uniformLayouts;
		
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
		//
		std::vector<uint16_t>						m_vecBufferOffsets;
		uint16_t									m_UBOLocalSize;
	public:
		const UniformBlock* getUniform(const std::string& _name) const;
		const UniformBlock::Member* getUniformMember(uint32_t _binding, const std::string& _name) const;
		const CombinedImageSampler* getSampler(const std::string& _name) const;
		const ShaderStorageBufferObject* getSSBO(const std::string& _name) const;
		const TexelBufferObject* getTBO(const std::string& _name) const;
		const SubpassInput* getSubpassInput(const std::string& _name) const;
		//
		void completeSetup( ContextVk* _context );
		//
		void setupDescriptorBufferInfo();
		void setupDescriptorImageInfo();
	};

	//
	//struct ArgumentLayout {
	//	struct UniformMember {
	//		std::string	name;
	//		uint32_t	binding;
	//		uint32_t	offset;
	//	};
	//	uint32_t						m_descriptorSetIndex;
	//	VkDescriptorSetLayout			m_descriptorSetLayout;
	//	//std::vector<ShaderDescriptor>	m_descriptors;
	//	std::vector<ShaderDescriptor>	m_uniformBlockDescriptor;
	//	std::vector<ShaderDescriptor>	m_samplerImageDescriptor;
	//	std::vector<ShaderDescriptor>	m_storageBufferDescriptor;
	//	std::vector<ShaderDescriptor>	m_texelBufferDescriptor;
	//	std::vector< uint32_t >			m_dynamicalBindings;
	//	std::vector<UniformMember>		m_uniformMembers;
	//	///////
	//	const ShaderDescriptor* getUniformBlock(const std::string& _name);
	//	uint32_t getUniformBlockMemberOffset( uint32_t _binding, const std::string& _name );
	//	const ShaderDescriptor* getSampler(const std::string& _name );
	//	const ShaderDescriptor* getSSBO(const std::string& _name );
	//	void updateDynamicalBindings();
	//};

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
			: m_descriptorSetLayoutCount( 0 )
			, m_argumentLayouts()
			, m_pipelineLayout( VK_NULL_HANDLE )
		{
		}

		virtual IArgument* createArgument( uint32_t _index ) override;
		virtual void destroyArgument( IArgument* _argument ) override;

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
		static MaterialVk* CreateMaterial( ContextVk* _context, const MaterialDescription& _desc );
		static VkShaderModule CreateShaderModule(ContextVk* _context, const char * _shader, ShaderModuleType _type);
	};

}