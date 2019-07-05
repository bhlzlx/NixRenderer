#pragma once 
#include "VkInc.h"
#include <string>
#include <vector>
#include <set>
#include "DebuggerVk.h"
#include <NixRenderer.h>
#include <iostream>
#include <VkShaderCompiler/VkShaderCompiler.h>

namespace Nix {

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
		virtual void debug( const char * _text) override {
			std::cout<< "debug :" << _text << std::endl;
		}
		virtual void warning(const char* _text) override {
			std::cout << "warning :" << _text << std::endl;
		}
		virtual void error(const char* _text) override {
			std::cout << "error :" << _text << std::endl;
		}
	};

	class DriverVk : public IDriver {
	private:
		VkInstance							m_instance;
		VkPhysicalDevice					m_PhDevice;
		VkDevice							m_device;
		VkPhysicalDeviceProperties			m_deviceProps;
		IArchieve*							m_archieve;
		DebugReporterVk						m_debugReport;
		//
		std::set<unsigned>					m_queueSet;
		// Uniform Allocator
		// Dynamic Buffer Allocator
		SimplerLogger						m_logger;
		//
		void*								m_compilerLibrary;
		PFN_INITIALIZE_SHADER_COMPILER		m_initializeShaderCompiler;
		PFN_FINALIZE_SHADER_COMPILER		m_finalizeShaderCompiler;
		PFN_COMPILE_GLSL_2_SPV				m_compileGLSL2SPV;
	public:
		DriverVk() :
			m_instance(VK_NULL_HANDLE)
			, m_PhDevice( VK_NULL_HANDLE )
			, m_archieve( nullptr )
		{
		}

		virtual bool initialize( Nix::IArchieve* _arch, DeviceType _type ) override;
		virtual void release() override;
		virtual IContext* createContext( void* _hwnd ) override;
		virtual inline IArchieve* getArchieve() override {
			return m_archieve;
		}
		virtual inline ILogger* getLogger() override {
			return &m_logger;
		}
		virtual bool checkFormatSupport(NixFormat _format, FormatFeatureFlags _flags) override;
		virtual NixFormat selectDepthFormat(bool _highp) override;

	private:
		VkInstance createInstance();
		bool selectPhysicalDevice(DeviceType _type);
	public:
		VkSurfaceKHR createSurface( void* _hwnd );
		VkDevice createDevice( const std::vector<VkDeviceQueueCreateInfo>& _queue );
		unsigned requestGraphicsQueue( VkSurfaceKHR _surface );
		unsigned requestTransferQueue();
		//
		bool validatePipelineCache(const void * _data, size_t _length);
		inline VkInstance getInstance() { return m_instance; }
		inline const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() { return m_deviceProps; }
		bool compileGLSL2SPV(ShaderModuleType _type, const char * _text, const uint32_t** _spvBytes, size_t* _length, const char** _compileLog);
	};
}
