#include "DDSLoader.h"
#define TINYDDSLOADER_IMPLEMENTATION
#include "tinyddsloader.h"

namespace nix {
	//
	static_assert(sizeof(DDS_HEADER) == 124, "size does not match");
	
	TextureVk* TextureVk::createTextureDDS(const void * _data, size_t _length) {
		tinyddsloader::DDSFile file;
		auto result = file.Load((const uint8_t*)_data, _length);
		if (result != tinyddsloader::Success) {
			return nullptr;
		}
		TextureDescription desc = {}; {
			auto dim = file.GetTextureDimension();
			switch (dim)
			{
			case tinyddsloader::DDSFile::TextureDimension::Unknown:
				return nullptr;
			case tinyddsloader::DDSFile::TextureDimension::Texture1D:
				desc.type = Texture1D;
				break;
			case tinyddsloader::DDSFile::TextureDimension::Texture2D:
				if (file.IsCubemap()) {
					desc.type = TextureCube;
					if (file.GetArraySize() != 6 ) 
					{
						desc.type = TextureCubeArray;
					}
					else
					{
						desc.type = TextureCube;
					}
				}
				else
				{
					if (file.GetArraySize() > 1) {
						desc.type = Texture2DArray;
					}
					else {
						desc.type = Texture2D;
					}
				}
				break;
			case tinyddsloader::DDSFile::TextureDimension::Texture3D:
				desc.type = Texture3D;
				break;
			default:
				break;
			}
			desc.width = file.GetWidth();
			desc.height = 1;
			desc.depth = 1;
			if (desc.type == Texture2D || desc.type == TextureCube  || desc.type == Texture2DArray || desc.type == TextureCubeArray ) {
				desc.height = file.GetHeight();
				desc.depth = file.GetArraySize();
			}
			else if (desc.type == Texture3D) {
				desc.height = file.GetHeight();
				desc.depth = file.GetDepth();
			}
			//
			desc.mipmapLevel = file.GetMipCount();
			auto format = file.GetFormat();
			if (format == tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm) {
				desc.format = KsBC3_LINEAR_RGBA;
			}
			else if (format == tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm) {
				desc.format = KsBC1_LINEAR_RGBA;
			}
			else {
				return nullptr;
			}
		}
		TextureVk* texture = TextureVk::createTexture(VK_NULL_HANDLE, VK_NULL_HANDLE, desc, nix::TextureUsageTransferDestination | nix::TextureUsageSampled);
		//
		for (uint32_t mipIndex = 0; mipIndex < file.GetMipCount(); ++mipIndex)
		{
			uint32_t width = desc.width >> mipIndex;
			uint32_t height = desc.height >> mipIndex;
			if (desc.format == KsBC1_LINEAR_RGBA || desc.format == KsBC3_LINEAR_RGBA) {
				if (width <= 4) {
					width = 4;
				}
				if (height <= 4) {
					height = 4;
				}
			}
			uint32_t dataLength = width * height * file.GetBitsPerPixel(file.GetFormat()) / 8;
			for (uint32_t arrayIndex = 0; arrayIndex < file.GetArraySize(); ++arrayIndex) {
				auto layerData = file.GetImageData(mipIndex, arrayIndex);
				TextureRegion region = {}; {
					region.mipLevel = mipIndex;
					region.baseLayer = arrayIndex;
					region.offset = { 0 , 0, 0 };
					region.size = { width, height, 1 };
				}
				texture->setSubData(layerData->m_mem, dataLength, region);
			}
		}
		return texture;
	}
}