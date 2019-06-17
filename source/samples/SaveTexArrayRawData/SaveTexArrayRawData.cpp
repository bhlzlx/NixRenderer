#include <NixApplication.h>
#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <cstdio>

#include <EGL/eglplatform.h>
#include <GLES3/gl32.h>

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

class SaveTexArrayRawData : public NixApplication {
	virtual bool initialize( void* _wnd, Nix::IArchieve* _arch ) {
        printf("%s", "SaveTexArrayRawData is initializing!");
		Nix::IFile* file = _arch->open("texture/texture_array_etc2.ktx");
		Nix::IFile* mem = Nix::CreateMemoryBuffer(file->size());
		file->read(file->size(), mem);
		file->release();

		const uint8_t * ptr = (const uint8_t *)mem->constData();
		const uint8_t * end = ptr + mem->size();
		//
		KtxHeader* header = (KtxHeader*)ptr;
		ptr += sizeof(KtxHeader);

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
		//
		ptr += header->bytesOfKeyValueData;
		//
		uint32_t bpp = 8;
		uint32_t pixelsize = bpp / 8;

		// should care about the alignment of the cube slice & mip slice
		// but reference to the ETC2 & EAC & `KTX format reference`, for 4x4 block compression type, the alignment should be zero
		// so we can ignore the alignment
		// read all mip level var loop!
		auto* contentStart = ptr;
		uint32_t pixelContentSize = 0;
		for (uint32_t mipLevel = 0; mipLevel < header->mipLevelCount; ++mipLevel) {
			uint32_t mipBytes = *(uint32_t*)(ptr);
			pixelContentSize += mipBytes / header->arraySize;
			ptr += sizeof(mipBytes);
			ptr += mipBytes;
		}
		char * content = new char[pixelContentSize];
		ptr = contentStart;
		char * contentR = content;
		for (uint32_t mipLevel = 0; mipLevel < header->mipLevelCount; ++mipLevel) {
			uint32_t mipBytes = *(uint32_t*)(ptr);
			ptr += sizeof(mipBytes);
			memcpy(contentR, ptr, mipBytes / header->arraySize);
			ptr += mipBytes;
			contentR += mipBytes / header->arraySize;
		}
		FILE* f = fopen("layerRaw.bin", "wb+");
		fwrite(content, 1, pixelContentSize, f);
		fclose(f);
		delete[]content;
		return true;
    }
    
	virtual void resize(uint32_t _width, uint32_t _height) {
        printf("resized!");
    }

	virtual void release() {
        printf("destroyed");
    }

	virtual void tick() {
    }

	virtual const char * title() {
        return "hello,world!";
    }
	
	virtual uint32_t rendererType() {
		return 0;
	}
};

SaveTexArrayRawData theapp;

NixApplication* GetApplication() {
    return &theapp;
}