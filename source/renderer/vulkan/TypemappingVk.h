#pragma once
#include <NixRenderer.h>
#include "vkinc.h"

namespace Ks {

	static inline uint32_t KsFormatBits(KsFormat _format)
	{
		switch (_format)
		{
		case KsInvalidFormat: return 0;
		case KsRGBA8888_UNORM:
		case KsBGRA8888_UNORM:
		case KsRGBA8888_SNORM:
			return 32;
		case KsRGB565_PACKED:
		case KsRGBA5551_PACKED:
		case KsRGBA_F16:
			return 16;
		case KsRGBA_F32:
			return 128;
		case KsDepth24FStencil8:
		case KsDepth32F:
			return 32;
		case KsDepth32FStencil8:
			return 40;
		case KsETC2_LINEAR_RGBA:
		case KsEAC_RG11_UNORM:
			return 8; // 8 bit per pixel
		case KsBC1_LINEAR_RGBA:
			return 4; // it's not 4!!!!!!!!!!!!!!
		case KsBC3_LINEAR_RGBA:
			return 8;
		case KsPVRTC_LINEAR_RGBA:
			return 4; // pvrt 4bpp
		default:
			break;
		}
		return 0;
	}

	static inline VkFormat KsFormatToVk(KsFormat _format) {
		switch (_format) {
		case KsRGBA8888_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case KsRGBA8888_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
		case KsBGRA8888_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
		case KsRGB565_PACKED: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case KsRGBA5551_PACKED: return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
		case KsRGBA_F16: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case KsRGBA_F32: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case KsDepth24FStencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case KsDepth32F: return VK_FORMAT_D32_SFLOAT;
		case KsDepth32FStencil8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case KsETC2_LINEAR_RGBA: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		case KsEAC_RG11_UNORM: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
		case KsBC1_LINEAR_RGBA: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case KsBC3_LINEAR_RGBA: return VK_FORMAT_BC3_UNORM_BLOCK;
		case KsPVRTC_LINEAR_RGBA: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
		default:
			break;
		}
		return VK_FORMAT_UNDEFINED;
	}

	static inline VkSamplerAddressMode KsAddressModeToVk( AddressMode _mode)
	{
		switch (_mode)
		{
		case AddressModeWrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case AddressModeClamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case AddressModeMirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	static inline KsFormat VkFormatToKs( VkFormat _format )
	{
		switch (_format) {
		case VK_FORMAT_R8G8B8A8_UNORM: return KsRGBA8888_UNORM;
		case VK_FORMAT_B8G8R8A8_UNORM: return KsBGRA8888_UNORM;
		case VK_FORMAT_R8G8B8A8_SNORM: return KsRGBA8888_SNORM;
		case VK_FORMAT_R5G6B5_UNORM_PACK16: return KsRGB565_PACKED;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return KsRGBA5551_PACKED;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return KsRGBA_F16;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return KsRGBA_F32;
		case VK_FORMAT_D24_UNORM_S8_UINT: return KsDepth24FStencil8;
		case VK_FORMAT_D32_SFLOAT: return KsDepth32F;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return KsDepth32FStencil8;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return KsETC2_LINEAR_RGBA;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return KsEAC_RG11_UNORM;
		case VK_FORMAT_BC3_UNORM_BLOCK: return KsBC3_LINEAR_RGBA;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK : return KsBC1_LINEAR_RGBA;
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return KsPVRTC_LINEAR_RGBA;
		default:
			break;
		}
		return KsInvalidFormat;
	}

	static inline VkFilter KsFilterToVk(TextureFilter _filter)
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

	static inline VkSamplerMipmapMode KsMipmapFilerToVk( TextureFilter _filter)
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

	static inline VkCullModeFlags GxCullModeToVk( CullMode _mode)
	{
		switch (_mode)
		{
		case None:
			return VK_CULL_MODE_NONE;
		case Back:
			return VK_CULL_MODE_BACK_BIT;
		case Front:
			return VK_CULL_MODE_FRONT_BIT;
		case FrontAndBack:
			return VK_CULL_MODE_FRONT_AND_BACK;
		}
		return VK_CULL_MODE_NONE;
	}

	static inline VkPrimitiveTopology KsTopologyToVk( TopologyMode _mode )
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

	static inline VkPolygonMode KsTopolygyPolygonMode( TopologyMode _mode )
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

	static inline VkFrontFace GxFrontFaceToVk( WindingMode _mode)
	{
		switch (_mode)
		{
		case Clockwise: return VK_FRONT_FACE_CLOCKWISE;
		case CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}

	static inline VkCompareOp KsCompareOpToVk( CompareFunction _op)
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

	static inline VkStencilOp KsStencilOpToVk( StencilOperation _op)
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

	static inline VkAttachmentLoadOp KsLoadOpToVk( RTLoadAction _op )
	{
		switch (_op)
		{
		case Keep:return VK_ATTACHMENT_LOAD_OP_LOAD;
			break;
		case Clear:return VK_ATTACHMENT_LOAD_OP_CLEAR;
			break;
		case DontCare:VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			break;
		}
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	}

	// our engine does not have the [store] action
	// so, the default op is [store op : store]
	// static VkAttachmentStoreOp GxStoreOpToVk(GX_STORE_ACTION _op);

	static inline VkBlendFactor KsBlendFactorToVk( BlendFactor _factor)
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

	static inline VkBlendOp KsBlendOpToVk( BlendOperation _op)
	{
		switch (_op)
		{
		case BlendOpAdd: return VK_BLEND_OP_ADD;
		case BlendOpSubtract: return VK_BLEND_OP_SUBTRACT;
		case BlendOpRevsubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
		}
		return VK_BLEND_OP_ADD;
	}

	static inline VkFormat KsVertexFormatToVK( VertexType _type)
	{
		switch (_type)
		{
		case VertexTypeFloat1: return VK_FORMAT_R32_SFLOAT;
		case VertexTypeFloat2: return VK_FORMAT_R32G32_SFLOAT;
		case VertexTypeFloat3: return VK_FORMAT_R32G32B32_SFLOAT;
		case VertexTypeFloat4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case VertexTypeHalf2: return VK_FORMAT_R16G16_SFLOAT;
		case VertexTypeHalf4: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case VertexTypeUByte4: return VK_FORMAT_R8G8B8A8_UINT;
		case VertexTypeUByte4N: return VK_FORMAT_R8G8B8A8_UNORM;
		}
		return VK_FORMAT_UNDEFINED;
	}
}
