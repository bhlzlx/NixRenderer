#pragma once
#include <cstdint>
#include <map>
#include <list>
#include <cassert>
#include <vector>
#include <NixRenderer.h>
#include "TypemappingVk.h"
#include "VkInc.h"
#include "vk_mem_alloc.h"

namespace Nix {
	class ContextVk;
	class TransientBuffer {
	private:
		uint32_t							m_alignment;
		VkBuffer							m_buffer;
		VmaAllocation						m_vmaAllocation;
		uint32_t							m_bufferSize;
		uint32_t							m_flightIndex;
		//
		std::map<VertexType, VkBufferView>	m_bufferViews;
		VertexType							m_cachedBufferViewType;
		VkBufferView						m_cachedBufferView;
		//z
		uint32_t							m_flightOffsets[MaxFlightCount];
		uint32_t							m_position;
		uint32_t							m_free;
	public:
		TransientBuffer()
			: m_alignment(32)
			, m_buffer(VK_NULL_HANDLE)
			, m_bufferSize(0)
			, m_flightIndex(-1)
			, m_cachedBufferViewType( VertexType::VertexTypeInvalid)
			, m_cachedBufferView(VK_NULL_HANDLE)
			, m_position(0)
			, m_free(0)
		{
		}
		TransientBuffer(TransientBuffer&& _buffer) {
			m_alignment = _buffer.m_alignment;
			m_buffer = _buffer.m_buffer;
			m_vmaAllocation = _buffer.m_vmaAllocation;
			m_bufferSize = _buffer.m_bufferSize;
			m_flightIndex = _buffer.m_flightIndex;
			m_bufferViews = _buffer.m_bufferViews;
			m_cachedBufferViewType = _buffer.m_cachedBufferViewType;
			m_cachedBufferView = _buffer.m_cachedBufferView;
			memcpy(m_flightOffsets, _buffer.m_flightOffsets, sizeof(m_flightOffsets));
			m_position = _buffer.m_position;
			m_free = _buffer.m_free;
		}
		void initialize(VkBuffer _buffer, VmaAllocation _allocation, uint32_t _size) {
			m_buffer = _buffer;
			m_vmaAllocation = _allocation;
			m_bufferSize = _size;
			for (uint32_t i = 0; i < MaxFlightCount; ++i) {
				m_flightOffsets[i] = (_size / MaxFlightCount) * i;
			}
		}
		void tick() {
			m_flightIndex += 1;
			m_flightIndex %= MaxFlightCount;
			m_position = m_flightIndex * m_bufferSize / MaxFlightCount;
			m_free = m_bufferSize / MaxFlightCount;
		}
		bool allocate(uint32_t _size, uint32_t& _offset, VkBuffer& _buffer) {
			if (m_free >= _size) {
				_offset = m_position;
				m_position += _size;
				m_free -= _size;
				_buffer = m_buffer;
				return true;
			}
			return false;
		}
		VkBufferView getBufferView( VkDevice _device, VertexType _type ) {
			if (m_cachedBufferViewType == _type) {
				return m_cachedBufferView;
			}
			auto it = m_bufferViews.find(_type);
			if ( it != m_bufferViews.end()) {
				return it->second;
			}
			VkBufferViewCreateInfo info;
			info.buffer = m_buffer;
			info.flags = 0;
			info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			info.pNext = nullptr;
			info.range = VK_WHOLE_SIZE;
			info.offset = 0;
			info.format = NixVertexFormatToVK(_type);
			VkBufferView view;
			if (VK_SUCCESS != vkCreateBufferView(_device, &info, nullptr, &view)) {
				assert(false);
				m_bufferViews[_type] = VK_NULL_HANDLE;
				return VK_NULL_HANDLE;
			}
			m_bufferViews[_type] = view;
			m_cachedBufferView = view;
			m_cachedBufferViewType = _type;
			return view;
		}
	};

	class TransientBufferAllocator {
	public:
		enum TransientType {
			UniformType,		// uniform buffer / uniform texel buffer
			StorageType,		// storage buffer / storage texel buffer
		};
	private:
		std::vector<TransientBuffer> m_vecTransientBuffer;
		ContextVk*		m_context;
		TransientType	m_type;
		uint32_t		m_blockSize;
		//
		uint32_t		m_vecIndex;
	private:
		bool createBuffer(TransientType _type, uint32_t _blockSize, VkBuffer& _buffer, VmaAllocation& _allocation);
	public:
		TransientBufferAllocator() {
		}
		bool initialize(ContextVk* _context, uint32_t _blockSize, TransientType _type);
		void allocate(uint32_t _size, uint32_t& _offset, VkBuffer& _buffer, VertexType _vertexType = VertexType::VertexTypeInvalid, VkBufferView* _bufferView = nullptr );
		void tick() {
			for (auto& buffer : m_vecTransientBuffer) {
				buffer.tick();
			}
			m_vecIndex = 0;
		}
	};

	class DescriptorSetBufferAllocator {
	private:
		TransientBufferAllocator m_uniformBuffer;
		TransientBufferAllocator m_storageBuffer;
	public:
		DescriptorSetBufferAllocator() {
		}
		bool initialize(ContextVk* _context);
		void allocateUniform(uint32_t _size, uint32_t& _offset, VkBuffer& _buffer, VertexType _vertexType = VertexType::VertexTypeInvalid, VkBufferView* _bufferView = nullptr);
		void allocateStorage(uint32_t _size, uint32_t& _offset, VkBuffer& _buffer, VertexType _vertexType = VertexType::VertexTypeInvalid, VkBufferView* _bufferView = nullptr);
	};
}