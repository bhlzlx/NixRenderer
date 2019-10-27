#include "DriverVk.h"
#include "VkInc.h"
#include "vkhelper/helper.h"
#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include "TypemappingVk.h"
#include <string.h>
#include <map>

#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef __ANDROID__
#include <dlfcn.h>
#elif defined __linux__
#include <dlfcn.h>
#include <X11/Xlib.h>
#endif

#ifdef _WIN32
#define VULKAN_LIBRARY_NAME "vulkan-1.dll"
#define SHADER_COMPILER_LIBRARY_NAME "VkShaderCompiler.dll"
#else
#define VULKAN_LIBRARY_NAME "libvulkan.so"
#define SHADER_COMPILER_LIBRARY_NAME "libVkShaderCompiler.so"
#endif

//#ifdef _WIN32
//#define OpenLibrary( name ) ::LoadLibraryA( name )
//#define CloseLibrary( library ) ::FreeLibrary( (HMODULE) library)
//#define GetExportAddress( libray, function ) ::GetProcAddress( (HMODULE)libray, function )
//#else
//#define OpenLibrary( name ) dlopen(name , RTLD_NOW | RTLD_LOCAL)
//#define CloseLibrary( library ) dlclose((void*)library)
//#define GetLibraryAddress( libray, function ) dlsym( (void*)libray, function )
//#endif

#undef REGIST_VULKAN_FUNCTION
#ifdef _WIN32
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(GetProcAddress( (HMODULE)library, #FUNCTION ));
#elif defined __ANDROID__
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(dlsym( library, #FUNCTION ));
#elif __linux__ 
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(dlsym( library, #FUNCTION ));
#endif

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#elif defined __ANDROID__
#include <vulkan/vulkan_android.h>
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#elif __linux__
#include <vulkan/vulkan_xlib.h>
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#endif

#ifdef _WIN32
#include <Windows.h>
#define OpenLibrary( name ) (void*)::LoadLibraryA(name)
#define CloseLibrary( library ) ::FreeLibrary((HMODULE)library)
#define GetExportAddress( libray, function ) ::GetProcAddress( (HMODULE)libray, function )
#else
#include <dlfcn.h>
#define OpenLibrary( name ) dlopen(name , RTLD_NOW | RTLD_LOCAL)
#define CloseLibrary( library ) dlclose((void*)library)
#define GetExportAddress( libray, function ) dlsym( (void*)libray, function )
#endif

namespace Nix {

	bool DriverVk::initialize( IArchieve* _arch, DeviceType _type) {
		auto library = OpenLibrary(VULKAN_LIBRARY_NAME);
		if (library == NULL) return false;
#include "vulkan_api.h"
#ifdef _WIN32
		REGIST_VULKAN_FUNCTION(vkCreateWin32SurfaceKHR)
#elif defined __ANDROID__
		REGIST_VULKAN_FUNCTION(vkCreateAndroidSurfaceKHR)
#elif defined __linux__
        REGIST_VULKAN_FUNCTION(vkCreateXlibSurfaceKHR)
#endif		
		if (!vkGetInstanceProcAddr || !vkCreateInstance || !vkAcquireNextImageKHR) {
			return false;
		}
#ifndef NDEBUG
		struct ApiItem {
			const char* name;
			const void* const address;
		};
        
#undef REGIST_VULKAN_FUNCTION
#define REGIST_VULKAN_FUNCTION(FUNCTION) { #FUNCTION, (const void* const)FUNCTION },
        
		ApiItem apiList[] = {
            
			#include "vulkan_api.h"
            
		};
#endif
		this->m_instance = createInstance();
		this->m_archieve = _arch;
		// setup debug
		m_debugReport.setupDebugReport(m_instance);
		//
		if (!selectPhysicalDevice(_type)) {
			return false;
		}
		vkGetPhysicalDeviceProperties(m_PhDevice, &m_deviceProps);
//////////////////////////////////////////////////////////////////////////
		m_compilerLibrary = (void*)OpenLibrary(SHADER_COMPILER_LIBRARY_NAME);
		if (m_compilerLibrary == NULL)
			return false;
		m_getShaderCompiler = reinterpret_cast<PFN_GETSHADERCOMPILER>(GetExportAddress(m_compilerLibrary, "GetVkShaderCompiler"));		
		m_shaderCompiler = m_getShaderCompiler();

		return true;
	}

	void DriverVk::release() {
		m_shaderCompiler->release();
		if (m_compilerLibrary) {
			CloseLibrary(m_compilerLibrary);
			m_compilerLibrary = nullptr;
		}
	}

	bool DriverVk::checkFormatSupport(NixFormat _format, FormatFeatureFlags _flags)
	{
		VkFormat format = NixFormatToVk(_format);
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhDevice, format, &props);
		uint32_t supportFlags = 0;
		if (_flags & FormatFeatureFlagBits::FeatureColorAttachment) supportFlags |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		if (_flags & FormatFeatureFlagBits::FeatureDepthStencilAttachment) supportFlags |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (_flags & FormatFeatureFlagBits::FeatureSampling) supportFlags |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		if (_flags & FormatFeatureFlagBits::FeatureTransferSource) supportFlags |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
		if (_flags & FormatFeatureFlagBits::FeatureTransferDestination) supportFlags |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
		if (_flags & FormatFeatureFlagBits::FeatureBlitSource) supportFlags |= VK_FORMAT_FEATURE_BLIT_SRC_BIT;
		if (_flags & FormatFeatureFlagBits::FeatureBlitDestination) supportFlags |= VK_FORMAT_FEATURE_BLIT_DST_BIT;

		if (_flags & FormatFeatureFlagBits::FeatureOptimalTiling) {
			if ( (supportFlags & props.optimalTilingFeatures) == supportFlags ) {
				return true;
			}
		}else {
			if ((supportFlags & props.linearTilingFeatures) == supportFlags) {
				return true;
			}
		}
		return false;
	}

	VkSurfaceKHR DriverVk::createSurface(void* _hwnd)
	{
		VkSurfaceKHR surface;
		VkResult rst = VK_ERROR_INVALID_EXTERNAL_HANDLE;
#ifdef _WIN32
		HINSTANCE hInst = ::GetModuleHandle(0);
		//
		VkWin32SurfaceCreateInfoKHR surface_create_info = {
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,  // VkStructureType                  sType
			nullptr,                                          // const void                      *pNext
			0,                                                // VkWin32SurfaceCreateFlagsKHR     flags
			hInst,
			(HWND)_hwnd
		};
		rst = vkCreateWin32SurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
#elif defined __ANDROID__
		VkAndroidSurfaceCreateInfoKHR surface_create_info = {
			VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,  // sType
			nullptr,// pNext
			0, // flag
			(ANativeWindow*)_hwnd
		};
		rst = vkCreateAndroidSurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
#elif defined __linux__
		VkXlibSurfaceCreateInfoKHR surface_create_info = {
			VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,  // sType
			nullptr,// pNext
			0, // flag
			(Display*)_hwnd
		};
		rst = vkCreateXlibSurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
#endif
		if (rst != VK_SUCCESS){
			return VK_NULL_HANDLE;
		}
		return surface;
	}

	VkDevice DriverVk::createDevice(const std::vector<VkDeviceQueueCreateInfo>& _queue)
	{
		std::vector< const char* > layers;
		std::vector<VkLayerProperties> vecLayerProps;
		std::vector< const char* > exts;
		std::vector<VkExtensionProperties> vecExtsProps;
		uint32_t count;
		vkEnumerateDeviceLayerProperties( m_PhDevice, &count, nullptr);
		if (count) {
			vecLayerProps.resize(count);
			vkEnumerateDeviceLayerProperties(m_PhDevice, &count, &vecLayerProps[0]);
		}
		for (auto& layer : vecLayerProps){
			layers.push_back(layer.layerName);
		}

		count = 0;
		vkEnumerateDeviceExtensionProperties(m_PhDevice, nullptr, &count, nullptr);
		vecExtsProps.resize(count);
		vkEnumerateDeviceExtensionProperties(m_PhDevice, nullptr, &count, &vecExtsProps[0]);
		for (auto& ext : vecExtsProps)
		{
			exts.push_back(ext.extensionName);
		}
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures( m_PhDevice, &features);

		const char * deviceExts[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		const char* mgdLayer = "VK_LAYER_ARM_MGD";
		VkDeviceCreateInfo deviceCreateInfo = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // VkStructureType sType
			nullptr, // const void *pNext
			0, // VkDeviceCreateFlags flags
			static_cast<uint32_t>( _queue.size()), // uint32_t queueCreateInfoCount
			& _queue[0], // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
			0,//1,// deviceLayers.size(),//0,
			nullptr,//&mgdLayer,//deviceLayers.data(),// nullptr,
			1, // uint32_t enabledExtensionCount
			&deviceExts[0], // const char * const *ppEnabledExtensionNames
			&features
		};
		//
		VkDevice device;
		if (VK_SUCCESS == vkCreateDevice( m_PhDevice, &deviceCreateInfo, nullptr, &device))
		{
			return device;
		}
		return VK_NULL_HANDLE;
	}

	VkInstance DriverVk::createInstance()
	{
		VkApplicationInfo applicationInfo; {
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.pNext = nullptr;
			applicationInfo.pApplicationName = APPLICATION_NAME;
			applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
			applicationInfo.pEngineName = ENGINE_NAME;
			applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		}

		// ============= Get Layer List =============
		static const auto LayerList = {
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_LUNARG_device_limits",
			"VK_LAYER_LUNARG_image",
			"VK_LAYER_LUNARG_swapchain",
			"VK_LAYER_GOOGLE_threading",
			"VK_LAYER_GOOGLE_unique_objects",
			"VK_LAYER_LUNARG_standard_validation",
		};
		std::vector< const char* > vecLayers;
		// not all pre defined layers are supported by your graphics card
		std::vector< VkLayerProperties  > vecLayerProps;
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (count) {
			vecLayerProps.resize(count);
			vkEnumerateInstanceLayerProperties(&count, &vecLayerProps[0]);
		}
		for (auto& layer : vecLayerProps)
		{
			for (auto& enableLayer : LayerList)
			{
				if (strcmp(layer.layerName, enableLayer) == 0)
				{
					vecLayers.push_back( enableLayer );
				}
			}
		}
		// ============= Get Extention List =============
		 count = 0;
		std::vector< VkExtensionProperties > vecExtProps;
		std::vector<const char*> exts;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		if (count) {
			vecExtProps.resize(count);
			vkEnumerateInstanceExtensionProperties(nullptr, &count, &vecExtProps[0]);
		}
		for (auto& ext : vecExtProps) {
			exts.push_back(ext.extensionName);
		}
		// ============= Create Instance Object =============
		VkInstanceCreateInfo info; {
			info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			info.flags = 0;
			info.pNext = nullptr;
			info.pApplicationInfo = &applicationInfo;
			info.enabledLayerCount = static_cast<uint32_t>(vecLayers.size());
			info.ppEnabledLayerNames = vecLayers.data();
			info.enabledExtensionCount = static_cast<uint32_t>(exts.size());
			info.ppEnabledExtensionNames = exts.data();
		}
		//
		VkInstance instance;
		vkCreateInstance(&info, nullptr, &instance);
		return instance;
	}

	bool DriverVk::selectPhysicalDevice(DeviceType _type)
	{
		uint32_t physicalDeviceCount = 0;
		std::vector<VkPhysicalDevice>  physicalDevices;
		auto rst = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
		if (rst != VK_SUCCESS || physicalDeviceCount == 0) {
			return false;
		}
		physicalDevices.resize(physicalDeviceCount);
		rst = vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, &physicalDevices[0]);
		// select a device that match the device type
		std::vector< VkPhysicalDeviceProperties > vecProps;
		for (auto& phyDevice : physicalDevices)
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(phyDevice, &props);
			uint32_t major_version = VK_VERSION_MAJOR(props.apiVersion);
			uint32_t minor_version = VK_VERSION_MINOR(props.apiVersion);
			uint32_t patch_version = VK_VERSION_PATCH(props.apiVersion);
			if (major_version >= 1 && props.deviceType == (VkPhysicalDeviceType)_type) {
				m_deviceProps = props;
				m_PhDevice = phyDevice;
				return true;
			}
			vecProps.push_back(props);
		}
		// cannot find a device match the device type
		m_PhDevice = physicalDevices[0];
		vkGetPhysicalDeviceProperties(m_PhDevice, &m_deviceProps);
		return true;
	}

	unsigned DriverVk::requestGraphicsQueue( VkSurfaceKHR _surface )
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( m_PhDevice, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0) {
			return -1;
		}
		std::vector< VkQueueFamilyProperties > familyProperties;
		familyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties( m_PhDevice, &queueFamilyCount, familyProperties.data());
		//
		for (uint32_t familyIndex = 0; familyIndex < familyProperties.size(); ++familyIndex)
		{
			const VkQueueFamilyProperties& property = familyProperties[familyIndex];
			if (property.queueCount && property.queueFlags & VK_QUEUE_GRAPHICS_BIT && property.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				VkBool32 supportPresent = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR( m_PhDevice, familyIndex, _surface, &supportPresent);
				if (supportPresent == VK_TRUE)
				{
					QueueReqData req;
					req.family = static_cast<uint16_t>(familyIndex);
					for (uint16_t index = 0; index < property.queueCount; ++index)
					{
						req.index = index;
						if ( m_queueSet.find(req.id) == m_queueSet.end())
						{
							m_queueSet.insert(req.id);
							return req.id;
						}
					}
				}
			}
		}
		return -1;
	}

	unsigned DriverVk::requestTransferQueue()
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( m_PhDevice, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0) {
			return -1;
		}
		std::vector< VkQueueFamilyProperties > familyProperties;
		familyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties( m_PhDevice, &queueFamilyCount, familyProperties.data());
		//
		for (uint32_t familyIndex = 0; familyIndex < familyProperties.size(); ++familyIndex)
		{
			const VkQueueFamilyProperties& property = familyProperties[familyIndex];
			if (property.queueCount && (property.queueFlags & VK_QUEUE_TRANSFER_BIT || property.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				QueueReqData req;
				req.family = static_cast<uint16_t>(familyIndex);
				for (uint16_t index = 0; index < property.queueCount; ++index)
				{
					req.index = index;
					if (m_queueSet.find(req.id) == m_queueSet.end())
					{
						m_queueSet.insert(req.id);
						return req.id;
					}
				}
			}
		}
		return -1;
	}

	Nix::NixFormat DriverVk::selectDepthFormat(bool _highp)
	{
		NixFormat HighP[] = {
			NixDepth32FStencil8, NixDepth32F, NixDepth24FStencil8, NixDepth24FX8, NixDepth16F
		};
		if (_highp) {
			for (int i = 0; i < sizeof(HighP) / sizeof(HighP[0]); ++i)
				if (checkFormatSupport(
					HighP[i],
					FormatFeatureFlagBits::FeatureOptimalTiling |
					FormatFeatureFlagBits::FeatureDepthStencilAttachment |
					FormatFeatureFlagBits::FeatureSampling)) {
					return HighP[i];
				}
		}
		else
		{
			for (int i = sizeof(HighP) / sizeof(HighP[0]) - 1; i >= 0 ; --i)
				if (checkFormatSupport(
					HighP[i],
					FormatFeatureFlagBits::FeatureOptimalTiling |
					FormatFeatureFlagBits::FeatureDepthStencilAttachment |
					FormatFeatureFlagBits::FeatureSampling)) {
					return HighP[i];
				}
		}
		return NixInvalidFormat;
	}

	bool DriverVk::validatePipelineCache(const void * _data, size_t _length)
	{
		struct alignas(4) PipelineCacheHeader {
			uint32_t headerSize;
			uint32_t headerVersion;
			uint32_t vendorID;
			uint32_t deviceID;
			char     cacheID[VK_UUID_SIZE];
		};
		if (_length <= sizeof(PipelineCacheHeader)) {
			return false;
		}
		const PipelineCacheHeader* header = ( const PipelineCacheHeader*)_data;
		if (header->headerSize != sizeof(PipelineCacheHeader) ) {
			return false;
		}
		if (header->headerVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE) {
			return false;
		}
		if (header->vendorID != m_deviceProps.vendorID ) {
			return false;
		}
		if (header->deviceID != m_deviceProps.deviceID) {
			return false;
		}
		if (memcmp(header->cacheID, m_deviceProps.pipelineCacheUUID, sizeof(m_deviceProps.pipelineCacheUUID)) != 0) {
			return false;
		}
		return true;
	}

	spvcompiler::IShaderCompiler* DriverVk::getShaderCompiler() {
		return m_shaderCompiler;
	}
}

extern "C" {
	NIX_API_DECL Nix::IDriver* createDriver() {
		Nix::DriverVk* driver = new Nix::DriverVk();
		return driver;
	}
}

