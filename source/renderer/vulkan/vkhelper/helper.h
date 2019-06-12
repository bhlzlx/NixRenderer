#pragma once
#include "../VkInc.h"
#ifdef _WIN32
#include <Windows.h>
#endif
//#include <SDL/SDL.h>

#ifdef VK_NO_PROTOTYPES
#else
#endif

#include <vector>

namespace vkhelper
{
// 	VkInstance createInstanceEasy();
// 	VkSurfaceKHR createSurface( VkInstance _inst,  void* _wnd );
// 	VkPhysicalDevice selectPhysicalDevice( VkInstance _inst, uint32_t _phyIndex = 0 );
// 	//
// 	uint32_t requestGraphicQueue(VkPhysicalDevice _device, VkSurfaceKHR _surface );
// 	uint32_t requestTransferQueue(VkPhysicalDevice _device);
// 
// 	std::vector< VkDeviceQueueCreateInfo > createDeviceQueueCreateInfo( std::vector< uint32_t > _queues );
// 	//
// 	VkDevice createDevice( VkPhysicalDevice _physical, const std::vector< VkDeviceQueueCreateInfo >& _queueInfo );

	uint32_t getMemoryType(VkPhysicalDevice _device, uint32_t _memTypeBits, VkMemoryPropertyFlags properties);

	VkBool32 isDepthFormat( VkFormat _format );
	VkBool32 isStencilFormat( VkFormat _format );
	void getImageAcessFlagAndPipelineStage(VkImageLayout _layout, VkAccessFlags& _flags, VkPipelineStageFlags& _stages);
	//
}
