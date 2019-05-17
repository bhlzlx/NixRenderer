#pragma once
#include "vkinc.h"
#include <KsRenderer.h>
#include <list>
#include <array>

namespace Ks {

	class BufferVk;
	class TextureVk;
	class RenderPassVk;
	class ArgumentVk;
	class PipelineVk;

	class QueueAsyncTask
	{
	public:
		virtual void execute() = 0;
	};

	class KS_API_DECL GraphicsQueueAsyncTaskManager
	{
	private:
		QueueAsyncTask * createDestroyTask(BufferVk * _buffer);
		QueueAsyncTask * createDestroyTask(TextureVk* _texture);
		QueueAsyncTask * createDestroyTask(RenderPassVk* _renderPass);
		QueueAsyncTask * createDestroyTask(ArgumentVk* _drawable);
		QueueAsyncTask * createDestroyTask(PipelineVk* _pipeline);
		//
		std::array< std::list<QueueAsyncTask*>, MaxFlightCount >  m_vecItems;
		uint32_t m_flightIndex;
	public:
		GraphicsQueueAsyncTaskManager();
		~GraphicsQueueAsyncTaskManager();
		//
		template<class T>
		void destroyResource(T _object) {
			// create deferred destroy item & add to destroy list
			QueueAsyncTask* item = this->createDestroyTask(_object);
			if (item) {
				m_vecItems[m_flightIndex].push_back(item);
			}
		}

		void addCaptureTask() {

		}

		void tick(uint32_t _flightIndex) {
			auto& itemList = m_vecItems[_flightIndex];
			for (auto& item : itemList) {
				item->execute();
			}
			//
			itemList.clear();
			//
			m_flightIndex = _flightIndex;
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
