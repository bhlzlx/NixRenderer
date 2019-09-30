#pragma once
#include <cstdint>
#include <map>
#include <list>
#include <cassert>
#include <vector>
#include <NixRenderer.h>
#include "VkInc.h"

namespace Nix {
	class ContextVk;
	class TransientBuffer {
		struct RingBufferRange {
			uint32_t begin;
			uint32_t end;
		};
	private:
		uint32_t m_alignment;
		VkBuffer m_buffer;
		VkBufferView m_imageViews;
		uint32_t m_bufferSize;
		uint32_t m_flightIndex;
		//
		uint32_t m_flightOffsets[MaxFlightCount];
		uint32_t m_position;
		uint32_t m_free;
	public:
		TransientBuffer()
			: m_alignment(32)
			, m_buffer(VK_NULL_HANDLE)
			, m_flightIndex(-1)
		{
		}
		void initialize(VkBuffer _buffer, uint32_t _size) {
			m_buffer = _buffer;
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
	};

	class TransientBufferAllocator {
	private:
		std::vector<TransientBuffer> m_vecTransientBuffer;
	public:
		enum TransientType {
			UniformType, // uniform buffer
			StorageType, // storage buffer
			TexelType, // texel buffer
		};
		ContextVk*		m_context;
		TransientType	m_type;
		uint32_t		m_blockSize;
	private:
		bool createBuffer(TransientType _type, uint32_t _blockSize, VkBuffer& _buffer, VmaAllocation& _allocation);
	public:
		TransientBufferAllocator() {
		}
		void initialize(ContextVk* _context, uint32_t _blockSize, TransientType _type);
		void allocate(uint32_t _size, uint32_t& _offset, VkBuffer& _buffer );
		void tick() {
			for (auto& buffer : m_vecTransientBuffer) {
				buffer.tick();
			}
		}
	};
}