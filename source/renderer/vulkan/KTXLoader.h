#include "TextureVk.h"

namespace Ks {
#pragma pack( push, 1 )
	struct KtxHeader {
		uint8_t		idendifier[12];
		uint32_t	endianness;
		uint32_t	type;
		uint32_t	typeSize;
		uint32_t	format;
		uint32_t	internalFormat;
		uint32_t	baseInternalFormat;
		uint32_t	pixelWidth;
		uint32_t	pixelHeight;
		uint32_t	pixelDepth;
		uint32_t	arraySize;
		uint32_t	faceCount;
		uint32_t	mipLevelCount;
		uint32_t	bytesOfKeyValueData;
	};
#pragma pack( pop )

	struct KtxLoader {
	private:
		VkFormat m_format;
		uint32_t m_width;
		uint32_t m_heigth;
		uint32_t m_depth;
		uint32_t m_mipLevelCount;
		uint32_t m_arrayLength;
		//
		TextureVk* m_texture;
	public:
		static TextureVk* CreateTexture(const void* _data, size_t _length);
	};

}