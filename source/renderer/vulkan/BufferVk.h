#pragma once
#include <NixRenderer.h>
#include "VkInc.h"
#ifdef _MINWINDEF_
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif
#include <vk_mem_alloc.h>
#include <vector>
#include <cassert>
#include "bufferDef.h"


// destroy a buffer object will call `delete` right now
// but the BufferVk resource handler will move to `deferred deleter`
//
// base type : BufferVk

// vertex buffer / index buffer / storage buffer / texel buffer
// uniform buffer / staging buffer

namespace Nix {
	// global vulkan memory allocator
	class ContextVk;
	class BufferAllocatorVk;

	class NIX_API_DECL BufferVk : public IBuffer {
		friend class CommandBufferVk;
		friend class GraphicsQueueAsyncTaskManager;
		friend struct ScreenCapture;
	private:
		ContextVk*			m_context;
		BufferAllocatorVk*	m_allocator;
		BufferAllocation	m_allocation;
		VkBufferUsageFlags	m_usage;
		//
	private:
		BufferVk(const BufferVk&) noexcept
			: IBuffer(m_type) {
			assert(false && "never call this method!");
		}
		BufferVk& operator=(const BufferVk&) {
			assert(false && "never call this method!");
			return *this;
		}
	public:
		BufferVk() noexcept
			: IBuffer(InvalidBufferType)
			, m_context(nullptr)
			, m_allocator(nullptr)
			, m_usage(0)
		{
		}

		BufferVk(ContextVk* _context, const BufferAllocation& _allocation, BufferAllocatorVk* _allocator, BufferType _type, VkBufferUsageFlags _usage)
			: IBuffer(_type)
			, m_context(_context)
			, m_allocation(_allocation)
			, m_allocator(_allocator)
			, m_usage(_usage) {
		}
		BufferVk(BufferVk&& _buffer) noexcept : IBuffer(m_type) {
			m_context = _buffer.m_context;
			m_usage = _buffer.m_usage;
			m_allocation = _buffer.m_allocation;
			m_allocator = _buffer.m_allocator;
			_buffer.m_context = VK_NULL_HANDLE;
			_buffer.m_usage = 0;
			_buffer.m_allocation.allocationId = -1;
			_buffer.m_allocation.buffer = 0;
			_buffer.m_allocator = nullptr;
		}
		BufferVk& operator = (BufferVk&& _buffer) noexcept {
			m_context = _buffer.m_context;
			m_usage = _buffer.m_usage;
			m_allocation = _buffer.m_allocation;
			_buffer.m_context = VK_NULL_HANDLE;
			_buffer.m_usage = 0;
			_buffer.m_allocation;
			return *this;
		}

		operator const VkBuffer&() const {
			return (VkBuffer&)m_allocation.buffer;
		}

		VkBuffer getHandle() {
			return (VkBuffer)m_allocation.buffer;
		}

		size_t getOffset() {
			return m_allocation.offset;
		}

		~BufferVk() {

		}
		//
		uint8_t* raw() {
			return (uint8_t*)m_allocation.raw;
		}
		// for persistent mapping or can be mapped buffer type
		void writeDataImmediatly(const void * _data, size_t _size, size_t _offset);
		// for GPU access only buffer
		void uploadDataImmediatly(const void * _data, size_t _size, size_t _offset);
		// common buffer 
		void updateDataQueued(const void * _data, size_t _size, size_t _offset);
		//
		size_t size() const {
			return static_cast<size_t>(m_allocation.size);
		}

		// 通过 IBuffer 继承
		virtual size_t getSize() override;
		virtual void initData(const void * _data, size_t _size, size_t _offset) override;
		virtual void updateData(const void * _data, size_t _size, size_t _offset) override;
		virtual void release() override;
		//static BufferVk CreateBuffer(size_t _size, VkBufferUsageFlags _usage, ContextVk* _context);
	};

}
