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

	struct Binding {
		ShaderDescriptorType	type = SDT_UniformBlock;
		std::string				name;
	};

	struct StageIOAttribute {
		uint16_t location;
		VertexType type;
		std::string  name;
	};

	struct SubpassInput {
		uint16_t set;
		uint16_t binding;
		uint16_t inputIndex;
	};

	struct PushConstants {
		uint16_t offset;
		uint16_t size;
	};

	struct UniformBlock {
		struct Member {
			uint16_t offset;
			uint16_t size;
			std::string name;
		};
		uint16_t set;
		uint16_t binding;
		uint16_t size;
		std::string name;
	};

	struct ShaderStorageBufferObject {
		uint16_t set;
		uint16_t binding;
		std::string name;
		size_t size;
	};

	struct TexelBufferObject {
		uint16_t set;
		uint16_t binding;
		std::string name;
	};

	struct AtomicCounter {
		uint16_t set;
		uint16_t binding;
		std::string name;
	};

	struct CombinedImageSampler {
		uint16_t set;
		uint16_t binding;
		std::string name;
	};

	class ArgumentLayoutExt {
		using ArgumentUniformLayouts = std::map<uint32_t, std::vector<UniformBlock>>;
	private:
		uint32_t									m_setID;
		VkDescriptorSet								m_vkSet;
		//
		std::vector<SubpassInput>					m_vecSubpassInput;
		std::vector<UniformBlock>					m_vecUniformBlock;
		std::vector<ShaderStorageBufferObject>		m_vecSSBO;
		std::vector<TexelBufferObject>				m_vecTBO;
		std::vector<AtomicCounter>					m_vecAtomic;
		std::vector<CombinedImageSampler>			m_vecSampler;
		//
		ArgumentUniformLayouts						m_uniformLayouts;
	public:
		const UniformBlock* getUniform(const std::string& _name) const;
		const UniformBlock::Member* getUniformMember(uint32_t _binding, const std::string& _name) const;
		const CombinedImageSampler* getSampler(const std::string& _name) const;
		const ShaderStorageBufferObject* getSSBO(const std::string& _name) const;
		const TexelBufferObject* getTBO(const std::string& _name) const;
		const SubpassInput* getSubpassInput(const std::string& _name) const;
		void updateDynamicalBindings();
	};

	//
	struct ArgumentLayout {
		struct UniformMember {
			std::string	name;
			uint32_t	binding;
			uint32_t	offset;
		};
		uint32_t						m_descriptorSetIndex;
		VkDescriptorSetLayout			m_descriptorSetLayout;
		//std::vector<ShaderDescriptor>	m_descriptors;
		std::vector<ShaderDescriptor>	m_uniformBlockDescriptor;
		std::vector<ShaderDescriptor>	m_samplerImageDescriptor;
		std::vector<ShaderDescriptor>	m_storageBufferDescriptor;
		std::vector<ShaderDescriptor>	m_texelBufferDescriptor;
		std::vector< uint32_t >			m_dynamicalBindings;
		std::vector<UniformMember>		m_uniformMembers;
		///////
		const ShaderDescriptor* getUniformBlock(const std::string& _name);
		uint32_t getUniformBlockMemberOffset( uint32_t _binding, const std::string& _name );
		const ShaderDescriptor* getSampler(const std::string& _name );
		const ShaderDescriptor* getSSBO(const std::string& _name );
		void updateDynamicalBindings();
	};

	class MaterialVk : public IMaterial {
		friend class ArgumentAllocator;
	private:
		MaterialDescription										m_description;
		ContextVk*												m_context;
		VkShaderModule											m_vertexShader;
		VkShaderModule											m_fragmentShader;
		uint32_t												m_descriptorSetLayoutCount;
		std::array<ArgumentLayout, MaxArgumentCount>			m_argumentLayouts;
		//
		VkPipelineLayout										m_pipelineLayout;
		VkPrimitiveTopology										m_topology;
		VkPolygonMode											m_pologonMode;
		//
		uint32_t												m_constantsStageFlags;
	public:
		MaterialVk() :
			  m_vertexShader( VK_NULL_HANDLE )
			, m_fragmentShader( VK_NULL_HANDLE)
			, m_descriptorSetLayoutCount( 0 )
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

		const ArgumentLayout& getDescriptorSetLayout(uint32_t _setIndex) const {
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
		static VkShaderModule CreateShaderModule( ContextVk* _context, const char * _shader, ShaderModuleType _type, std::vector<uint32_t>& _spvBytes );
	};

}