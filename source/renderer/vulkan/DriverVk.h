#pragma once
#include "vkinc.h"
#include <string>
#include <vector>
#include <set>
#include "DebuggerVk.h"
#include <KsRenderer.h>
#include <iostream>

namespace Ks {

	class ContextVk;
	class IArchieve;

	static const char APPLICATION_NAME[] = "KUSUGAWA VULKAN WRAPPER";
	static const char ENGINE_NAME[] = "KUSUGAWA RENDERER";

#pragma pack( push, 2 )
	struct QueueReqData {
		union {
			struct {
				short family;
				short index;
			};
			unsigned id;
		};
	};
#pragma pack( pop )

	class SimplerLogger : public ILogger {
	public:
		virtual void debug(const std::string& _text) override {
			std::cout<< "debug :" << _text << std::endl;
		}
		virtual void warning(const std::string& _text) override {
			std::cout << "warning :" << _text << std::endl;
		}
		virtual void error(const std::string& _text) override {
			std::cout << "error :" << _text << std::endl;
		}
	};

	class DriverVk : IDriver {
	private:
		VkInstance m_instance;
		VkPhysicalDevice m_device;
		VkPhysicalDeviceProperties m_deviceProps;
		IArchieve* m_archieve;
		DebugReporterVk m_debugReport;
		//
		std::set<unsigned> m_queueSet;
		// Uniform Allocator
		// Dynamic Buffer Allocator
		SimplerLogger m_logger;
	public:
		DriverVk() :
			m_instance(VK_NULL_HANDLE)
			, m_device( VK_NULL_HANDLE )
			, m_archieve( nullptr )
		{

		}

		virtual bool initialize(IArchieve* _arch, DeviceType _type) override;
		virtual IContext* createContext( void* _hwnd ) override;
		virtual inline IArchieve* getArchieve() override {
			return m_archieve;
		}
		virtual inline ILogger* getLogger() override {
			&m_logger;
		}

	private:
		VkInstance createInstance();
		bool selectPhysicalDevice(DeviceType _type);
	public:
		VkSurfaceKHR createSurface( void* _hwnd );
		VkDevice createDevice( const std::vector<VkDeviceQueueCreateInfo>& _queue );
		unsigned requestGraphicsQueue( VkSurfaceKHR _surface );
		unsigned requestTransferQueue();
		//
		inline VkInstance getInstance() { return m_instance; }
		inline const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() { return m_deviceProps; }
	};

	DriverVk* CreateVulkanDriver();
}