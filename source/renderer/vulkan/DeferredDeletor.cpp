#include "DeferredDeletor.h"
#include "BufferVk.h"
#include "TextureVk.h"
#include "RenderPassVk.h"
#include "PipelineVk.h"
#include "ArgumentVk.h"
#include "BufferAllocator.h"

namespace Nix {

	GraphicsQueueAsyncTaskManager DeferredDeletor;

	template<>
	inline void GraphicsQueueAsyncTaskManager::destroyResource(BufferVk* _buffer) {
		QueueAsyncTask* item = this->createDestroyTask(_buffer->m_allocator, _buffer->m_allocation);
		if (item) {
			uint32_t freeIndex = (m_currentIndex + MaxFlightCount) % (MaxFlightCount + 1);
			m_vecItems[freeIndex].push_back(item);
		}
	}

	struct BufferDeleteItem : public QueueAsyncTask {
	private:
		BufferAllocation m_allocation;
		BufferAllocatorVk* m_allocator;
	public:
		BufferDeleteItem(BufferAllocatorVk* _allocator, BufferAllocation _allocation)
			: m_allocator(_allocator)
			, m_allocation(_allocation) {
		}
		//
		virtual void execute() {
			m_allocator->free(m_allocation);
			delete this;
		}
	};

	struct TextureDeleteItem : public QueueAsyncTask {
	private:
		TextureVk* m_texture;
	public:
		TextureDeleteItem(TextureVk* _texture) : m_texture(_texture) {
		}
		virtual void execute() {
			delete m_texture;
			delete this;
		}
	};

	struct RenderPassDeleteItem : public QueueAsyncTask {
	private:
		RenderPassVk* m_renderPass;
	public:
		RenderPassDeleteItem(RenderPassVk* _rp) : m_renderPass(_rp) {
		}
		virtual void execute() {
			delete m_renderPass;
			delete this;
		}
	};

	struct AttachmentDeleteItem : public QueueAsyncTask {
	private:
		AttachmentVk* m_attachment;
	public:
		AttachmentDeleteItem(AttachmentVk* _attachment) : m_attachment(_attachment) {
		}
		virtual void execute() {
			delete m_attachment;
			delete this;
		}
	};

	struct ArgumentDeleteItem : public QueueAsyncTask {
	private:
		ArgumentVk* m_argument;
	public:
		ArgumentDeleteItem(ArgumentVk* _arg) : m_argument(_arg) {
		}
		virtual void execute() {
			delete m_argument;
			delete this;
		}
	};

	struct PipelineDeleteItem : public QueueAsyncTask {
	private:
		PipelineVk* m_pipeline;
	public:
		PipelineDeleteItem(PipelineVk* _pipeline) : m_pipeline(_pipeline) {
		}
		virtual void execute() {
			delete m_pipeline;
			delete this;
		}
	};

	Nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(BufferAllocatorVk* _allocator, const BufferAllocation& _allocation)
	{
		return new BufferDeleteItem(_allocator, _allocation);
	}

	Nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(TextureVk* _texture)
	{
		return new TextureDeleteItem(_texture);
	}

	Nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(RenderPassVk* _renderPass)
	{
		return new RenderPassDeleteItem(_renderPass);
	}

	Nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(ArgumentVk* _argument)
	{
		return new ArgumentDeleteItem(_argument);
	}

	Nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(PipelineVk* _pipeline)
	{
		return new PipelineDeleteItem(_pipeline);
	}

	GraphicsQueueAsyncTaskManager::GraphicsQueueAsyncTaskManager() :
		m_currentIndex(0)
	{
	}

	GraphicsQueueAsyncTaskManager::~GraphicsQueueAsyncTaskManager()
	{
	}

}
