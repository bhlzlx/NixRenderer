#include "DriverVk.h"
#include "vkinc.h"
#include "vkhelper/helper.h"
#include <map>

#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef __ANDROID__
//#include 
#endif

#undef REGIST_VULKAN_FUNCTION
#ifdef _WIN32
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(GetProcAddress( library, #FUNCTION ));
#elif defined __ANDROID__
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(dlsym( library, #FUNCTION ));
#endif

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#elif defined __ANDROID__
#include <vulkan/vulkan_android.h>
PFN_vkCreateAndroidSurfaceKHR vkCreateWin32SurfaceKHR;
#endif

namespace Ks {

	DriverVk* CreateVulkanDriver() {
		DriverVk* driver = new DriverVk();
		return driver;
	}

	bool DriverVk::initialize(IArchieve* _arch, DeviceType _type ) {
#ifdef _WIN32
		HMODULE library = ::LoadLibraryA("vulkan-1.dll");
		if (library == NULL) {
			return false;
		}
#endif
#ifdef __ANDROID__
		void* library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
		if (!library) return false;
#endif
#include "vulkan_api.h"
#ifdef _WIN32
		REGIST_VULKAN_FUNCTION(vkCreateWin32SurfaceKHR)
#elif defined __ANDROID__
		REGIST_VULKAN_FUNCTION(vkCreateAndroidSurfaceKHR)
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
		vkGetPhysicalDeviceProperties(m_device, &m_deviceProps);
		//
		return true;
	}

	VkSurfaceKHR DriverVk::createSurface( void* _hwnd )
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
			(ANativeWindow*)_wnd
		};
		rst = vkCreateAndroidSurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
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
		vkEnumerateDeviceLayerProperties( m_device, &count, nullptr);
		if (count) {
			vecLayerProps.resize(count);
			vkEnumerateDeviceLayerProperties(m_device, &count, &vecLayerProps[0]);
		}
		for (auto& layer : vecLayerProps){
			layers.push_back(layer.layerName);
		}

		count = 0;
		vkEnumerateDeviceExtensionProperties(m_device, nullptr, &count, nullptr);
		vecExtsProps.resize(count);
		vkEnumerateDeviceExtensionProperties(m_device, nullptr, &count, &vecExtsProps[0]);
		for (auto& ext : vecExtsProps)
		{
			exts.push_back(ext.extensionName);
		}
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures( m_device, &features);

		const char* mgdLayer = "VK_LAYER_ARM_MGD";
		VkDeviceCreateInfo deviceCreateInfo = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // VkStructureType sType
			nullptr, // const void *pNext
			0, // VkDeviceCreateFlags flags
			static_cast<uint32_t>( _queue.size()), // uint32_t queueCreateInfoCount
			& _queue[0], // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
			0,//1,// deviceLayers.size(),//0,
			nullptr,//&mgdLayer,//deviceLayers.data(),// nullptr,
			static_cast<uint32_t>(exts.size()), // uint32_t enabledExtensionCount
			& exts[0], // const char * const *ppEnabledExtensionNames
			&features
		};
		//
		VkDevice device;
		if (VK_SUCCESS == vkCreateDevice( m_device, &deviceCreateInfo, nullptr, &device))
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
				m_device = phyDevice;
				return true;
			}
			vecProps.push_back(props);
		}
		// cannot find a device match the device type
		m_device = physicalDevices[0];
		vkGetPhysicalDeviceProperties(m_device, &m_deviceProps);
		return true;
	}

	unsigned DriverVk::requestGraphicsQueue( VkSurfaceKHR _surface )
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( m_device, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0) {
			return -1;
		}
		std::vector< VkQueueFamilyProperties > familyProperties;
		familyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties( m_device, &queueFamilyCount, familyProperties.data());
		//
		for (uint32_t familyIndex = 0; familyIndex < familyProperties.size(); ++familyIndex)
		{
			const VkQueueFamilyProperties& property = familyProperties[familyIndex];
			if (property.queueCount && property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				VkBool32 supportPresent = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR( m_device, familyIndex, _surface, &supportPresent);
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
		vkGetPhysicalDeviceQueueFamilyProperties( m_device, &queueFamilyCount, nullptr);
		if (queueFamilyCount == 0) {
			return -1;
		}
		std::vector< VkQueueFamilyProperties > familyProperties;
		familyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties( m_device, &queueFamilyCount, familyProperties.data());
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
}