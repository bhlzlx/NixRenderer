#include "DeferredDeletor.h"
#include "BufferVk.h"
#include "TextureVk.h"
#include "RenderPassVk.h"
#include "PipelineVk.h"
#include "ArgumentVk.h"

namespace nix {

	GraphicsQueueAsyncTaskManager DeferredDeletor;

	struct BufferDeleteItem : public QueueAsyncTask {
	private:
		BufferVk* m_buffer;
	public:
		BufferDeleteItem(BufferVk* _buffer) : m_buffer(_buffer) {
		}
		//
		virtual void execute() {
			delete m_buffer;
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

	nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(BufferVk * _buffer)
	{
		return new BufferDeleteItem(_buffer);
	}

	nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(TextureVk* _texture)
	{
		return new TextureDeleteItem(_texture);
	}

	nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(RenderPassVk* _renderPass)
	{
		return new RenderPassDeleteItem(_renderPass);
	}

	nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(ArgumentVk* _argument)
	{
		return new ArgumentDeleteItem(_argument);
	}

	nix::QueueAsyncTask * GraphicsQueueAsyncTaskManager::createDestroyTask(PipelineVk* _pipeline)
	{
		return new PipelineDeleteItem(_pipeline);
	}

	GraphicsQueueAsyncTaskManager::GraphicsQueueAsyncTaskManager()
	{
	}

	GraphicsQueueAsyncTaskManager::~GraphicsQueueAsyncTaskManager()
	{
	}

}