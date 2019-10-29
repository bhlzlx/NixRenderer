#pragma once
#include "VkInc.h"
#include <NixRenderer.h>
#include <list>
#include <array>
#include "bufferDef.h"

namespace Nix {

	class BufferVk;
	class TextureVk;
	class RenderPassVk;
	class ArgumentVk;
	class PipelineVk;
	class BufferAllocatorVk;

	class QueueAsyncTask
	{
	public:
		virtual void execute() = 0;
	};

	class NIX_API_DECL GraphicsQueueAsyncTaskManager
	{
	private:
		QueueAsyncTask * createDestroyTask(BufferAllocatorVk* _allocator, const BufferAllocation& _allocation);
		QueueAsyncTask * createDestroyTask(TextureVk* _texture);
		QueueAsyncTask * createDestroyTask(RenderPassVk* _renderPass);
		QueueAsyncTask * createDestroyTask(ArgumentVk* _drawable);
		QueueAsyncTask * createDestroyTask(PipelineVk* _pipeline);
		//
		std::array< std::list<QueueAsyncTask*>, MaxFlightCount + 1 >  m_vecItems;
		uint32_t m_currentIndex;
	public:
		GraphicsQueueAsyncTaskManager();
		~GraphicsQueueAsyncTaskManager();
		//
		template<class T>
		void destroyResource(T _object) {
			// create deferred destroy item & add to destroy list
			QueueAsyncTask* item = this->createDestroyTask(_object);
			if (item) {
				uint32_t freeIndex = (m_currentIndex + MaxFlightCount) % (MaxFlightCount + 1);
				m_vecItems[freeIndex].push_back(item);
			}
		}

		template<>
		void destroyResource<BufferVk*>(BufferVk* _buffer);

		void addCaptureTask() {

		}

		void tick() {
			auto& itemList = m_vecItems[m_currentIndex];
			for (auto& item : itemList) {
				item->execute();
			}
			itemList.clear();
			//
			m_currentIndex += 1;
			m_currentIndex %= (MaxFlightCount + 1);
		}
		//
		void cleanup() {
			for (auto& itemList : m_vecItems) {
				for (auto& item : itemList) {
					item->execute();
				}
				itemList.clear();
			}
		}
	};

	extern GraphicsQueueAsyncTaskManager DeferredDeletor;

	inline GraphicsQueueAsyncTaskManager& GetDeferredDeletor() {
		return DeferredDeletor;
	}
}
