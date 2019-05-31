#pragma once
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
// declare vulkan api - core
#undef REGIST_VULKAN_FUNCTION
#define REGIST_VULKAN_FUNCTION( FUNCTION ) extern PFN_##FUNCTION FUNCTION;
#include "vulkan_api.h"
//

enum DeviceType {
	DeviceTypeOthers,
	DeviceTypeIntegrated,
	DeviceTypeDiscrete,
	DeviceTypeVirtual,
	DeviceTypeCPU
};

namespace Ks {
	class BufferVk;
}

struct UBOAllocation {
	Ks::BufferVk* buffer;
	size_t offset;
	size_t capacity;
};