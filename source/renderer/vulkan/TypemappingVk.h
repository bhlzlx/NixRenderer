#pragma once
#include <NixRenderer.h>
#include "VkInc.h"

namespace Nix {

	static inline uint32_t NixFormatBits( NixFormat _format )
	{
		switch (_format)
		{
		case NixInvalidFormat: return 0;
		case NixRGBA8888_UNORM:
		case NixBGRA8888_UNORM:
		case NixRGBA8888_SNORM:
			return 32;
		case NixRGB565_PACKED:
		case NixRGBA5551_PACKED:
		case NixRGBA_F16:
			return 16;
		case NixRGBA_F32:
			return 128;
		case NixDepth24FStencil8:
		case NixDepth32F:
			return 32;
		case NixDepth32FStencil8:
			return 40;
		case NixETC2_LINEAR_RGBA:
		case NixEAC_RG11_UNORM:
			return 8; // 8 bit per pixel
		case NixBC1_LINEAR_RGBA:
			return 4; // it's not 4!!!!!!!!!!!!!!
		case NixBC3_LINEAR_RGBA:
			return 8;
		case NixPVRTC_LINEAR_RGBA:
			return 4; // pvrt 4bpp
		default:
			break;
		}
		return 0;
	}

	static inline VkFormat NixFormatToVk(NixFormat _format) {
		switch (_format) {
		case NixR8_UNORM: return VK_FORMAT_R8_UNORM;
		case NixR16_UNORM: return VK_FORMAT_R16_UNORM;
		//
		case NixRGBA8888_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case NixRGBA8888_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
		case NixBGRA8888_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
		case NixRGB565_PACKED: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case NixRGBA5551_PACKED: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case NixRGBA_F16: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case NixRGBA_F32: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case NixDepth16F: return VK_FORMAT_D16_UNORM;
		case NixDepth24FX8: return VK_FORMAT_X8_D24_UNORM_PACK32;
		case NixDepth24FStencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case NixDepth32F: return VK_FORMAT_D32_SFLOAT;
		case NixDepth32FStencil8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case NixETC2_LINEAR_RGBA: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		case NixEAC_RG11_UNORM: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
		case NixBC1_LINEAR_RGBA: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case NixBC3_LINEAR_RGBA: return VK_FORMAT_BC3_UNORM_BLOCK;
		case NixPVRTC_LINEAR_RGBA: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
		default:
			break;
		}
		return VK_FORMAT_UNDEFINED;
	}

	static inline VkSamplerAddressMode NixAddressModeToVk( AddressMode _mode)
	{
		switch (_mode)
		{
		case AddressModeWrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case AddressModeClamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case AddressModeMirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	static inline NixFormat VkFormatToNix( VkFormat _format )
	{
		switch (_format) {
		case VK_FORMAT_R8G8B8A8_UNORM: return NixRGBA8888_UNORM;
		case VK_FORMAT_B8G8R8A8_UNORM: return NixBGRA8888_UNORM;
		case VK_FORMAT_R8G8B8A8_SNORM: return NixRGBA8888_SNORM;
		case VK_FORMAT_R5G6B5_UNORM_PACK16: return NixRGB565_PACKED;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return NixRGBA5551_PACKED;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return NixRGBA_F16;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return NixRGBA_F32;
		case VK_FORMAT_D16_UNORM: return NixDepth16F;
		case VK_FORMAT_X8_D24_UNORM_PACK32: return NixDepth24FX8;
		case VK_FORMAT_D24_UNORM_S8_UINT: return NixDepth24FStencil8;
		case VK_FORMAT_D32_SFLOAT: return NixDepth32F;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return NixDepth32FStencil8;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return NixETC2_LINEAR_RGBA;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return NixEAC_RG11_UNORM;
		case VK_FORMAT_BC3_UNORM_BLOCK: return NixBC3_LINEAR_RGBA;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK : return NixBC1_LINEAR_RGBA;
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return NixPVRTC_LINEAR_RGBA;
		case VK_FORMAT_R8_UNORM: return NixR8_UNORM;
		case VK_FORMAT_R16_UNORM: return NixR16_UNORM;
		default:
			break;
		}
		return NixInvalidFormat;
	}

	static inline VkFilter NixFilterToVk(TextureFilter _filter)
	{
		switch (_filter)
		{
		case TexFilterNone:
		case TexFilterPoint:
			return VK_FILTER_NEAREST;
		case TexFilterLinear:
			return VK_FILTER_LINEAR;
		}
		return VK_FILTER_NEAREST;
	}

	static inline VkSamplerMipmapMode NixMipmapFilerToVk( TextureFilter _filter)
	{
		switch (_filter)
		{
		case TexFilterNone:
		case TexFilterPoint:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case TexFilterLinear:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

	static inline VkCullModeFlags NixCullModeToVk( CullMode _mode)
	{
		switch (_mode)
		{
		case CullNone:
			return VK_CULL_MODE_NONE;
		case CullBack:
			return VK_CULL_MODE_BACK_BIT;
		case CullFront:
			return VK_CULL_MODE_FRONT_BIT;
		case CullAll:
			return VK_CULL_MODE_FRONT_AND_BACK;
		}
		return VK_CULL_MODE_NONE;
	}

	static inline VkPrimitiveTopology NixTopologyToVk( TopologyMode _mode )
	{
		switch (_mode)
		{
		case TMPoints: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case TMLineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case TMLineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case TMTriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case TMTriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case TMTriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			break;
		}
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}

	static inline VkPolygonMode NixPolygonModeToVk(PolygonMode _mode) {
		switch (_mode) {
		case PolygonMode::PMFill:
			return VkPolygonMode::VK_POLYGON_MODE_FILL;
		case PolygonMode::PMLine:
			return VkPolygonMode::VK_POLYGON_MODE_LINE;
		case PolygonMode::PMPoint:
			return VkPolygonMode::VK_POLYGON_MODE_POINT;
		}
		return VkPolygonMode::VK_POLYGON_MODE_FILL;
	}

	static inline VkPolygonMode NixTopolygyPolygonMode( TopologyMode _mode )
	{
		switch (_mode)
		{
		case TMPoints:
			return VK_POLYGON_MODE_POINT;
		case TMLineStrip:
		case TMLineList:
			return VK_POLYGON_MODE_LINE;
		case TMTriangleStrip:
		case TMTriangleList:
		case TMTriangleFan:
			return VK_POLYGON_MODE_FILL;
		case TMCount:
			break;
		}
		return VK_POLYGON_MODE_LINE;
	}

	static inline VkFrontFace NixFrontFaceToVk( WindingMode _mode)
	{
		switch (_mode)
		{
		case Clockwise: return VK_FRONT_FACE_CLOCKWISE;
		case CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}

	static inline VkCompareOp NixCompareOpToVk( CompareFunction _op)
	{
		switch (_op)
		{
		case Never: return VK_COMPARE_OP_NEVER;
		case Less: return VK_COMPARE_OP_LESS;
		case Equal: return VK_COMPARE_OP_EQUAL;
		case LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case Greater: return VK_COMPARE_OP_GREATER;
		case GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case Always: return VK_COMPARE_OP_ALWAYS;
		}
		//VK_COMPARE_OP_NOT_EQUAL
		return VK_COMPARE_OP_ALWAYS;
	}

	static inline VkStencilOp NixStencilOpToVk( StencilOperation _op)
	{
		switch (_op)
		{
		case StencilOpKeep:return VK_STENCIL_OP_KEEP;
		case StencilOpZero: return VK_STENCIL_OP_ZERO;
		case StencilOpReplace: return VK_STENCIL_OP_REPLACE;
		case StencilOpIncrSat:return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case StencilOpDecrSat:return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case StencilOpInvert:return VK_STENCIL_OP_INVERT;
		case StencilOpInc:return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case StencilOpDec:return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		}
		return VK_STENCIL_OP_REPLACE;
	}

	static inline VkAttachmentLoadOp NixLoadOpToVk( AttachmentLoadAction _op )
	{
		switch (_op)
		{
		case Keep:return VK_ATTACHMENT_LOAD_OP_LOAD;
			break;
		case Clear:return VK_ATTACHMENT_LOAD_OP_CLEAR;
			break;
		case DontCare:return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			break;
		}
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	}

	// our engine does not have the [store] action
	// so, the default op is [store op : store]
	// static VkAttachmentStoreOp GxStoreOpToVk(GX_STORE_ACTION _op);

	static inline VkBlendFactor NixBlendFactorToVk( BlendFactor _factor)
	{
		switch (_factor)
		{
		case Zero: return VK_BLEND_FACTOR_ZERO;
		case One: return VK_BLEND_FACTOR_ONE;
		case SourceColor: return VK_BLEND_FACTOR_SRC_COLOR;
		case InvertSourceColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case SourceAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
		case InvertSourceAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case DestinationColor: return VK_BLEND_FACTOR_DST_COLOR;
		case InvertDestinationColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case DestinationAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
		case InvertDestinationAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case SourceAlphaSat: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		}
		return VK_BLEND_FACTOR_ONE;
	}

	static inline VkBlendOp NixBlendOpToVk( BlendOperation _op)
	{
		switch (_op)
		{
		case BlendOpAdd: return VK_BLEND_OP_ADD;
		case BlendOpSubtract: return VK_BLEND_OP_SUBTRACT;
		case BlendOpRevsubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		}
		return VK_BLEND_OP_ADD;
	}

	static inline VkFormat NixVertexFormatToVK( VertexType _type)
	{
		switch (_type)
		{
		case VertexTypeFloat: return VK_FORMAT_R32_SFLOAT;
		case VertexTypeFloat2: return VK_FORMAT_R32G32_SFLOAT;
		case VertexTypeFloat3: return VK_FORMAT_R32G32B32_SFLOAT;
		case VertexTypeFloat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		//
		case VertexTypeUint: return VK_FORMAT_R32_UINT;
		case VertexTypeUint2: return VK_FORMAT_R32G32_UINT;
		case VertexTypeUint3: return VK_FORMAT_R32G32B32_UINT;
		case VertexTypeUint4: return VK_FORMAT_R32G32B32A32_UINT;
		//
		case VertexTypeHalf: return VK_FORMAT_R16_SFLOAT;
		case VertexTypeHalf2: return VK_FORMAT_R16G16_SFLOAT;
		case VertexTypeHalf3: return VK_FORMAT_R16G16B16_SFLOAT;
		case VertexTypeHalf4: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case VertexTypeUByte4: return VK_FORMAT_R8G8B8A8_UINT;
		case VertexTypeUByte4N: return VK_FORMAT_R8G8B8A8_UNORM;
		}
		return VK_FORMAT_UNDEFINED;
	}
}
