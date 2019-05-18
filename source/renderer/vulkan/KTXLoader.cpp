#include "KTXLoader.h"
#include "TypemappingVk.h"
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>

namespace nix {

	nix::TextureVk* TextureVk::createTextureKTX(const void * _data, size_t _length)
	{
		return KtxLoader::CreateTexture(_data, _length);
		//return nullptr;
	}
    
	nix::TextureVk* KtxLoader::CreateTexture(const void* _data, size_t _length)
	{
		const uint8_t * ptr = (const uint8_t *)_data;
		const uint8_t * end = ptr + _length;
		//
		KtxHeader* header = (KtxHeader*)ptr;
		ptr += sizeof(KtxHeader);
		//

		TextureDescription desc = {};

		uint8_t FileIdentifier[12] = {
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};
		// validate identifier
		if (memcmp(header->idendifier, FileIdentifier, sizeof(FileIdentifier)) != 0) {
			return nullptr;
		}
		// For compressed textures, glType must equal 0.
		if (header->type != 0) {
			return nullptr;
		}
		//glTypeSize specifies the data type size that should be used whenendianness conversion is required for the texture data stored in thefile. If glType is not 0, this should be the size in bytes correspondingto glType. For texture data which does not depend on platform endianness,including compressed texture data, glTypeSize must equal 1.
		if (header->typeSize != 1) {
			return nullptr;
		}
		// For compressed textures, glFormat must equal 0.
		if (header->format != 0) {
			return nullptr;
		}
		// GL_COMPRESSED_RGBA8_ETC2_EAC GL_COMPRESSED_RG11_EAC
		if (header->internalFormat == GL_COMPRESSED_RGBA8_ETC2_EAC && header->baseInternalFormat == GL_RGBA) {
			// GL_COMPRESSED_RGBA8_ETC2_EAC
			desc.format = VkFormatToKs(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
			// 
		} else if (header->internalFormat == GL_COMPRESSED_RG11_EAC && header->baseInternalFormat == GL_RG) {
			// GL_COMPRESSED_RG11_EAC
			// VK_FORMAT_EAC_R11G11_UNORM_BLOCK
			desc.format = VkFormatToKs(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
		}
		else if (header->internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT && header->baseInternalFormat == GL_RGBA) {
			// GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
			// VK_FORMAT_BC1_RGBA_UNORM_BLOCK
			desc.format = VkFormatToKs(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
		}
		else if (header->internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT && header->baseInternalFormat == GL_RGBA) {
			// GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
			// VK_FORMAT_BC3_UNORM_BLOCK
			desc.format = VkFormatToKs(VK_FORMAT_BC3_UNORM_BLOCK);
		}
		else {
			// unsupported texture format
			return nullptr;
		}
		desc.mipmapLevel = header->mipLevelCount;
		if (header->faceCount == 6) {
			if (header->arraySize >= 1) {
				desc.type = TextureCubeArray;
				desc.depth = header->faceCount * header->arraySize;
			}
			else {
				desc.type = TextureCube;
				desc.depth = 6;
			}
		}
		else if( header->pixelDepth >=1 ) {
			desc.type = Texture3D;
		}
		else {
			if (header->arraySize >= 1) {
				desc.type = Texture2DArray;
				desc.depth = header->arraySize;
			}
			desc.type = Texture2D;
			desc.depth = 1;
		}
		desc.width = header->pixelWidth;
		desc.height = header->pixelHeight;
		TextureVk* texture = TextureVk::createTexture(VK_NULL_HANDLE, VK_NULL_HANDLE, desc, TextureUsageSampled | TextureUsageTransferDestination);
		//
		ptr += header->bytesOfKeyValueData;
		//
		uint32_t bpp = 8;
		uint32_t pixelsize = bpp / 8;

		// should care about the alignment of the cube slice & mip slice
		// but reference to the ETC2 & EAC & `KTX format reference`, for 4x4 block compression type, the alignment should be zero
		// so we can ignore the alignment

		// read all mip level var loop!
		for (uint32_t mipLevel = 0; mipLevel < header->mipLevelCount; ++mipLevel) {
			uint32_t mipBytes = *(uint32_t*)(ptr);
			ptr += sizeof(mipBytes);
			for( uint32_t arrayIndex = 0; arrayIndex < desc.depth; ++arrayIndex ) {
                TextureRegion region; {
                    region.baseLayer = arrayIndex;
                    region.mipLevel = mipLevel;
                    region.offset = { 0, 0, 0 };
                    region.size = { header->pixelWidth >> mipLevel, header->pixelHeight >> mipLevel, 1 };
                }
                texture->setSubData(ptr, mipBytes, region);
                ptr += mipBytes;
			}
		}
		return texture;
	}

}