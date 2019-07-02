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

	typedef uint64_t AllocationHandle;
	class BufferAllocator {
	private:
		VmaAllocation		m_allocation;
		VkDeviceSize		m_size;
		
	public:
		void initialize( size_t _size );
		void initialize( size_t _size, size_t _minSize );
		// 
		uint16_t allocate( size_t _size );
		void free( AllocationHandle );
	};

	class NIX_API_DECL BufferVk {
		//friend class VkDeferredDeletor;
		//friend class ContextVk;
		friend struct ScreenCapture;
		friend class CachedVertexBuffer;
		friend class IndexBuffer;

		typedef uint64_t NixAllocation;
	private:
		ContextVk*			m_context;
		VkBuffer			m_buffer;
		VkDeviceSize		m_offset;
		VkDeviceSize		m_size;
		VkBufferUsageFlags	m_usage;
		VmaAllocation		m_allocation;
		// m_raw != nullptr indicate that, this buffer support persistent mapping
		// but if m_raw == nullptr does not means it does not support mapping at all
		// you can try call the mapping function to map the buffer, but dont forget to call unmapp
		uint8_t*			m_raw;
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
		BufferVk() :
			m_buffer(VK_NULL_HANDLE),
			m_size(0),
			m_usage(0),
			m_raw(nullptr){
		}
		BufferVk(BufferVk&& _buffer) {
			m_context = _buffer.m_context;
			m_buffer = _buffer.m_buffer;
			m_size = _buffer.m_size;
			m_usage = _buffer.m_usage;
			m_allocation = _buffer.m_allocation;
			m_raw = _buffer.m_raw;
			_buffer.m_buffer = VK_NULL_HANDLE;
			_buffer.m_size = 0;
			_buffer.m_usage = 0;
			_buffer.m_allocation = VK_NULL_HANDLE;
			_buffer.m_raw = nullptr;
		}
		BufferVk& operator = (BufferVk&& _buffer) {
			m_buffer = _buffer.m_buffer;
			m_size = _buffer.m_size;
			m_usage = _buffer.m_usage;
			m_allocation = _buffer.m_allocation;
			m_raw = _buffer.m_raw;
			m_context = _buffer.m_context;
			_buffer.m_buffer = VK_NULL_HANDLE;
			_buffer.m_size = 0;
			_buffer.m_usage = 0;
			_buffer.m_allocation = VK_NULL_HANDLE;
			_buffer.m_raw = nullptr;
			return *this;
		}

		operator const VkBuffer&() const {
			return m_buffer;
		}
		~BufferVk();
		void* map();
		void unmap();
		uint8_t* raw() {
			return m_raw;
		}
		// for persistent mapping or can be mapped buffer type
		void writeDataImmediatly(const void * _data, size_t _size, size_t _offset);
		// for GPU access only buffer
		void uploadDataImmediatly(const void * _data, size_t _size, size_t _offset);
		// common buffer 
		void updateDataQueued(const void * _data, size_t _size, size_t _offset);
		//
		size_t size() const {
			return static_cast<size_t>(m_size);
		}
		static BufferVk CreateBuffer(size_t _size, VkBufferUsageFlags _usage, ContextVk* _context);
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
		//
		operator VkBuffer () const {
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
		CachedVertexBuffer( BufferVk&& _buffer, size_t _size ) : IBuffer(CVBO) {
			m_buffer = std::move(_buffer);
			m_size = _size;
			m_offsets[0] = 0;
			m_offsets[1] = m_size;
			m_offsets[2] = m_size * 2;
			m_mem = (uint8_t*)_buffer.map();
		}
	public:
		virtual size_t getSize() override;
		virtual void setData(const void* _data, size_t _size, size_t _offset) override;
		virtual void release() override;
		//
		operator VkBuffer () const {
			return m_buffer;
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
		virtual void release() override;
		//
		operator VkBuffer() const {
			return m_buffer;
		}
		size_t getOffset() {
			return 0;
		}
	};
}