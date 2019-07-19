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

// destroy a buffer object will call `delete` right now
// but the BufferVk resource handler will move to `deferred deleter`

namespace Nix {
	// global vulkan memory allocator
	class ContextVk;

	class NIX_API_DECL BufferVk {
		friend struct ScreenCapture;
		friend class CachedVertexBuffer;
		friend class IndexBuffer;
		friend class VertexBuffer;
	private:
		ContextVk*			m_context;
		IBufferAllocator*	m_allocator;
		BufferAllocation	m_allocation;
		VkBufferUsageFlags	m_usage;
		//
	private:
		BufferVk(const BufferVk&) {
			assert(false && "never call this method!");
		}
		BufferVk& operator=(const BufferVk&) {
			assert(false && "never call this method!");
			return *this;
		}
	public:
		BufferVk()
			:m_context(nullptr)
			,m_allocator(nullptr)
		{
		}
		BufferVk( ContextVk* _context, const BufferAllocation& _allocation, VkBufferUsageFlags _usage ) :
			m_allocation(_allocation),
			m_usage(_usage){
		}
		BufferVk(BufferVk&& _buffer) {
			m_context = _buffer.m_context;
			m_usage = _buffer.m_usage;
			m_allocation = _buffer.m_allocation;
			_buffer.m_context = VK_NULL_HANDLE;
			_buffer.m_usage = 0;
			_buffer.m_allocation;
		}
		BufferVk& operator = (BufferVk&& _buffer) {
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
		~BufferVk();
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
		//static BufferVk CreateBuffer(size_t _size, VkBufferUsageFlags _usage, ContextVk* _context);
	};

	class NIX_API_DECL VertexBuffer : public IBuffer {
		friend class PipelineVk;
		friend class DrawableVk;
	private:
		BufferVk m_buffer;
	public:
		VertexBuffer(BufferVk&& _buffer) : 
			IBuffer(SVBO),
			m_buffer(std::move(_buffer)) {
		}
		//
		virtual void setData(const void * _data, size_t _size, size_t _offset ) override;
		virtual size_t getSize() override {
			return m_buffer.size();
		}
		virtual void release() override;

		virtual IBufferAllocator* getAllocator() {
			return m_buffer.m_allocator;
		}
		//
		operator VkBuffer () const {
			return m_buffer;
		}
		operator BufferVk& () {
			return m_buffer;
		}
		size_t getOffset() {
			return 0;
		}
	};

	class CachedVertexBuffer : public IBuffer {
	private:
		BufferVk		m_buffer;
		uint8_t*		m_mem;
		size_t			m_size;
		uint64_t		m_frame;
		size_t			m_offsets[MaxFlightCount];
	public:
		CachedVertexBuffer( BufferVk&& _buffer ) : IBuffer(CVBO) {
			m_buffer = std::move(_buffer);
			m_size = _buffer.m_allocation.size / MaxFlightCount;
			m_offsets[0] = _buffer.m_allocation.offset;
			m_offsets[1] = _buffer.m_allocation.offset + m_size;
			m_offsets[2] = _buffer.m_allocation.offset + m_size * 2;
			m_mem = (uint8_t*)_buffer.m_allocation.raw;
		}
	public:
		virtual size_t getSize() override;
		virtual void setData(const void* _data, size_t _size, size_t _offset) override;
		virtual void release() override;
		//
		operator VkBuffer () const {
			return m_buffer;
		}
		operator BufferVk&() {
			return m_buffer;
		}
		virtual IBufferAllocator* getAllocator() {
			return m_buffer.m_allocator;
		}
		size_t getOffset() {
			return m_offsets[0];
		}
	};

	class NIX_API_DECL IndexBuffer : public IBuffer
	{
		friend class ContextVk;
		friend class DrawableVk;
		friend class PipelineVk;
	private:
		BufferVk m_buffer;
	public:
		IndexBuffer( BufferVk&& _buffer) : IBuffer(IBO),
			m_buffer(std::move(_buffer))
		{
		}
		virtual void setData( const void* _data, size_t _size, size_t _offset) override;
		
		virtual size_t getSize() override {
			return m_buffer.size();
		}
		
		virtual IBufferAllocator* getAllocator() {
			return m_buffer.m_allocator;
		}
		virtual void release() override;
		//
		operator VkBuffer() const {
			return m_buffer;
		}
		operator BufferVk&() {
			return m_buffer;
		}
		size_t getOffset() {
			return 0;
		}
	};
}