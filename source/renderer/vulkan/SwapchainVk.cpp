#include "SwapchainVk.h"
#include "ContextVk.h"
#include "BufferVk.h"
#include "TextureVk.h"
#include "RenderPassVk.h"
#include "TypemappingVk.h"

namespace nix {

	void SwapchainVk::resize(uint32_t _width, uint32_t _height)
	{
		m_size = { _width, _height };
		updateSwapchain();
	}

	void SwapchainVk::cleanup()
	{
		uint32_t m_imageIndex = -1;
		uint32_t m_flightIndex = 0;
	}

	VkFormat SwapchainVk::nativeFormat() const {
		return m_createInfo.imageFormat;
	}

	NixFormat SwapchainVk::format() const {
		return VkFormatToNix(m_createInfo.imageFormat);
	}

	bool SwapchainVk::acquireNextImage()
	{
		if (!m_swapchain) {
			return false;
		}
		auto device = m_context->getDevice();
		VkResult rst = vkAcquireNextImageKHR(device, m_swapchain, uint64_t(-1), m_nextImageAvailable, VK_NULL_HANDLE, &m_imageIndex);
		//
		switch (rst) {
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			//updateSwapchain();
			// abort this frame, even you re create a swapchain right now, the `vkAcquireNextImageKHR` also will return a `VK_ERROR_OUT_OF_DATE_KHR` error code
		default:
			m_imageIndex = -1;
			m_available = VK_FALSE;
			return false;
		}
		//
		m_available = VK_TRUE;
		((RenderPassSwapchainVk*)m_renderPass)->activeSubpass(m_imageIndex);
		return true;
	}

	VkBool32 SwapchainVk::avail() const
	{
		return m_available;
	}

	bool SwapchainVk::updateSwapchain()
	{
		VkDevice device = m_context->getDevice();
		vkQueueWaitIdle(m_graphicsQueue);
		//
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_context->getPhysicalDevice(), m_context->getSurface(), &surfaceCapabilities) != VK_SUCCESS) {
			return false;
		}
		//
		m_size.width = (m_size.width + 1)&~(1);
		m_size.height = (m_size.height + 1)&~(1);
		//
		if (m_size.width >= surfaceCapabilities.minImageExtent.width
			&& m_size.width <= surfaceCapabilities.maxImageExtent.width
			&& m_size.height >= surfaceCapabilities.minImageExtent.height
			&& m_size.height <= surfaceCapabilities.maxImageExtent.height
			) {
			m_createInfo.imageExtent = {
				(uint32_t)m_size.width, (uint32_t)m_size.height
			};
		}
		else {
			m_createInfo.imageExtent = surfaceCapabilities.minImageExtent;
			m_size.width = m_createInfo.imageExtent.width;
			m_size.height = m_createInfo.imageExtent.height;
		}
		//
		m_createInfo.surface = m_context->getSurface();
		// 1. create swapchain object
		m_createInfo.oldSwapchain = m_swapchain;
		if ( m_createInfo.imageExtent.width < 4 || m_createInfo.imageExtent.height < 4 )
		{
			return false;
		}
		//VkSwapchainKHR oldSwapchain = m_swapchain;
		if (vkCreateSwapchainKHR( device, &m_createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		{
			return false;
		}
		cleanup();
		//
		if (m_createInfo.oldSwapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, m_createInfo.oldSwapchain, nullptr);
		}
		// 2. retrieve the image attached on the swapchain
		uint32_t nImage = 0;
		vkGetSwapchainImagesKHR(device, m_swapchain, &nImage, nullptr);
		if (!nImage) {
			return false;
		}
		std::vector<VkImage> images(nImage);
		vkGetSwapchainImagesKHR(device, m_swapchain, &nImage, &images[0]);
		// 4. create framebuffers
		((RenderPassSwapchainVk*)m_renderPass)->update(images, m_size.width, m_size.height, m_createInfo.imageFormat);
		m_available = true;
		return true;
	}

	bool SwapchainVk::present()
	{
		VkPresentInfoKHR presentInfo = {
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,			// VkStructureType              sType
			nullptr,									// const void                  *pNext
			1,											// uint32_t                     waitSemaphoreCount
			&m_readyToPresent,								// const VkSemaphore           *pWaitSemaphores
			1,											// uint32_t                     swapchainCount
			&m_swapchain,								// const VkSwapchainKHR        *pSwapchains
			&m_imageIndex,								// const uint32_t              *pImageIndices
			nullptr										// VkResult                    *pResults
		};
		//
		VkResult result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

		switch (result) {
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			updateSwapchain();
			return false;
		default:
			return false;
		}

		++m_flightIndex;
		m_flightIndex = m_flightIndex % MaxFlightCount;

		return true;
	}

	nix::SwapchainVk SwapchainVk::CreateSwapchain( ContextVk* _context )
	{
		SwapchainVk swapchain;
		auto device = _context->getDevice();
		auto physicalDevice = _context->getPhysicalDevice();
		auto surface = _context->getSurface();
		//
		vkDeviceWaitIdle(device);
		//
		VkSwapchainCreateInfoKHR createInfo;
		VkSurfaceCapabilitiesKHR surfaceCapabilities;

		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
		{
			return swapchain;
		}
		uint32_t formatCount;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS)
		{
			return swapchain;
		}
		if (formatCount == 0)
			return swapchain;
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, &surfaceFormats[0]) != VK_SUCCESS)
		{
			return swapchain;
		}

		uint32_t nPresentMode;
		if ((vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &nPresentMode, nullptr) != VK_SUCCESS) ||
			(nPresentMode == 0))
		{
			return swapchain;
		}

		std::vector<VkPresentModeKHR> presentModes(nPresentMode);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &nPresentMode, presentModes.data()) != VK_SUCCESS)
		{
			return swapchain;
		}
		// 1.
		if (surfaceCapabilities.maxImageCount == 0)
		{
			createInfo.minImageCount = surfaceCapabilities.minImageCount < MaxFlightCount ? MaxFlightCount : surfaceCapabilities.minImageCount;
		}
		else
		{
			createInfo.minImageCount = surfaceCapabilities.maxImageCount < MaxFlightCount ? surfaceCapabilities.maxImageCount : MaxFlightCount;
		}
		//desiredImagesCount = max(surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
		// 2.
		VkSurfaceFormatKHR desiredFormat = surfaceFormats[0];
		if ((surfaceFormats.size() == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			desiredFormat = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
		}
		else
		{
			for (VkSurfaceFormatKHR &surfaceFormat : surfaceFormats)
			{
				if (surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
				{
					desiredFormat = surfaceFormat;
					break;
				}
			}
		}

		VkSurfaceTransformFlagBitsKHR desiredTransform;
		VkPresentModeKHR presentMode;

		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		else
			desiredTransform = surfaceCapabilities.currentTransform;
		// 6.
		// FIFO present mode is always available
		// MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
		for (VkPresentModeKHR &pmode : presentModes)
		{
			if (pmode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				presentMode = pmode; break;
			}
		}

		if (presentMode != VK_PRESENT_MODE_MAILBOX_KHR)
		{
			for (VkPresentModeKHR &pmode : presentModes)
			{
				if (pmode == VK_PRESENT_MODE_FIFO_KHR)
				{
					presentMode = pmode; break;
				}
			}
		}
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		//_swapchainCI.minImageCount = _swapchainCI.minImageCount;
		createInfo.imageFormat = desiredFormat.format;
		createInfo.imageColorSpace = desiredFormat.colorSpace;
		createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		createInfo.imageArrayLayers = 1;
		createInfo.clipped = VK_TRUE;
		createInfo.surface = surface;
		createInfo.flags = 0;
		createInfo.pNext = nullptr;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.presentMode = presentMode;
		createInfo.preTransform = desiredTransform;

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		for (uint32_t flag = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; flag <= VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR; flag <<= 1)
		{//VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
			if (flag & capabilities.supportedCompositeAlpha)
			{
				createInfo.compositeAlpha = (VkCompositeAlphaFlagBitsKHR)flag;
				break;
			}
		}
		//SwapchainVk swapchain;
		swapchain.m_createInfo = createInfo;
		swapchain.m_flightIndex = 0;
		swapchain.m_imageIndex = 0;
		swapchain.m_swapchain = VK_NULL_HANDLE;
		swapchain.m_graphicsQueue = (const VkQueue&)(*_context->getGraphicsQueue());
		//swapchain.m_renderPass = renderPass;
		swapchain.m_nextImageAvailable = _context->createSemaphore();
		swapchain.m_readyToPresent = _context->createSemaphore();
		swapchain.m_renderPass = new RenderPassSwapchainVk();
		swapchain.m_context = _context;
		//
		return swapchain;
	}

}