#include "TextureVk.h"

namespace Ks {

	struct DDS_PIXELFORMAT {
		uint32_t dwSize;
		uint32_t dwFlags;
		uint32_t dwFourCC;
		uint32_t dwRGBBitCount;
		uint32_t dwRBitMask;
		uint32_t dwGBitMask;
		uint32_t dwBBitMask;
		uint32_t dwABitMask;
	};
	typedef struct {
		uint32_t           dwSize;
		uint32_t           dwFlags;
		uint32_t           dwHeight;
		uint32_t           dwWidth;
		uint32_t           dwPitchOrLinearSize;
		uint32_t           dwDepth;
		uint32_t           dwMipMapCount;
		uint32_t           dwReserved1[11];
		DDS_PIXELFORMAT    ddspf;
		uint32_t           dwCaps;
		uint32_t           dwCaps2;
		uint32_t           dwCaps3;
		uint32_t           dwCaps4;
		uint32_t           dwReserved2;
	} DDS_HEADER;
	
	enum DDSFlags {
		DDSD_CAPS = 0x1,
		DDSD_HEIGHT = 0x2,
		DDSD_WIDTH = 0x4,
		DDSD_PITCH = 0x8,
		DDSD_PIXELFORMAT = 0x1000,
		DDSD_MIPMAPCOUNT = 0x20000,
		DDSD_LINEARSIZE = 0x80000,
		DDSD_DEPTH = 0x800000,
	};
}