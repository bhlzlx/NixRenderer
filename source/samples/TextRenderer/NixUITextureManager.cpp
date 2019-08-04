#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <NixJpDecl.h>

#include "NixFontTextureManager.h"
#include <nix/io/archieve.h>
#include <stb_truetype.h>
#include <stb_image.h>
#include "NixUITextureManager.h"

#ifdef _WIN32
#include <Windows.h>
#define OpenLibrary( name ) (void*)::LoadLibraryA(name)
#define CloseLibrary( library ) ::FreeLibrary((HMODULE)library)
#define GetExportAddress( libray, function ) ::GetProcAddress( (HMODULE)libray, function )
#undef GetObject
#else
#include <dlfcn.h>
#define OpenLibrary( name ) dlopen(name , RTLD_NOW | RTLD_LOCAL)
#define CloseLibrary( library ) dlclose((void*)library)
#define GetExportAddress( libray, function ) dlsym( (void*)libray, function )
#endif

namespace Nix {

	struct JsPackerSize {
		uint32_t w = 0;
		uint32_t h = 0;
		NIX_JSON(w, h)
	};

	struct JsPackerFrame {
		uint32_t x = 0;
		uint32_t y = 0;
		uint32_t w = 0;
		uint32_t h = 0;
		NIX_JSON(w, h, x, y)
	};

	struct JsTextureItem {
		std::string			filename;
		JsPackerFrame		frame;
		bool				rotated;
		NIX_JSON(filename, frame, rotated)
	};

	struct JsPackerStrucMeta {
		std::string			image;
		std::string			format;
		JsPackerSize		size;
		std::string			scale;
		NIX_JSON( image, format, size, scale )
	};

	struct JsPackerStruct {
		JsPackerStrucMeta				meta;
		std::vector< JsTextureItem >	frames;
		NIX_JSON(meta, frames)
	};

	bool UITextureManager::initialize(IContext* _context, IArchieve* _archieve, uint32_t _numFontLayer, uint32_t _numControlLayer, uint32_t _numNetworkLayer)
	{
		m_numFontLayer = _numFontLayer; m_numControlLayer = _numControlLayer; m_numNetworkLayer = _numNetworkLayer;
		uint32_t totalLayer = _numFontLayer + _numControlLayer + _numNetworkLayer;
		Nix::TextureDescription desc; {
			desc.depth = totalLayer;
			desc.width = UITextureSize;
			desc.height = UITextureSize;
			desc.format = NixRGBA8888_UNORM;
			desc.mipmapLevel = 1;
			desc.type = Texture2DArray;
		}		
		m_texture = _context->createTexture(desc, Nix::TextureUsageSampled | Nix::TextureUsageTransferDestination );
		// load `Texture Packer` module
		m_texturePackerLibrary = OpenLibrary("TexturePacker.dll");
		if (!m_texturePackerLibrary) { 
			return false;
		}
		PFN_CREATE_TEXTURE_PACKER createTexturePacker = (PFN_CREATE_TEXTURE_PACKER)GetExportAddress(m_texturePackerLibrary, "CreateTexturePacker");
		if (!createTexturePacker) {
			return false;
		}
		bool rst = m_fontTexManager.initialize(_context, _archieve, m_texture, createTexturePacker, 0, 1);
		if (!rst) {
			return false;
		}
		//
		m_imageTables.resize(_numControlLayer);
		m_freeControlLayer = _numFontLayer;
		m_archieve = _archieve;
		return true;
	}

	bool UITextureManager::addPackedImage(const char* _jsonTable, const char* _imageFile)
	{
		TextReader jsonText;
		if (!jsonText.openFile(m_archieve, _jsonTable)) {
			return false;
		}
		IFile* imageFile = m_archieve->open(std::string(_imageFile));
		if (!imageFile) 
			return false;
		IFile* imageMem = CreateMemoryBuffer(imageFile->size());
		imageFile->read(imageFile->size(), imageMem);
		imageFile->release();
		//
		int x, y, n;
		stbi_uc* rawData = stbi_load_from_memory((const stbi_uc*)imageMem->constData(), imageMem->size(), &x, &y, &n, 4);
		//
		assert( (x == y) && ( y == UITextureSize) );
		BufferImageUpload upload;
		upload.baseMipRegion.baseLayer = m_freeControlLayer;
		upload.baseMipRegion.mipLevel = 0;
		upload.baseMipRegion.offset = { 0, 0, 0 };
		upload.baseMipRegion.size = { UITextureSize, UITextureSize, 1 };
		upload.mipCount = 1;
		upload.data = rawData;
		upload.length = x * y * 4;
		upload.mipDataOffsets[0] = 0;
		m_texture->uploadSubData(upload);
		stbi_image_free(rawData);
		// parse json & save the file information to table
		JsPackerStruct js;
		js.parse(jsonText.getText());
		assert( (js.meta.size.h == js.meta.size.w) &&  (js.meta.size.w == UITextureSize) );
		
		m_freeControlLayer;
		std::map<std::string, UITexture>& table = m_imageTables[m_freeControlLayer-m_numFontLayer];
		for (auto& imageItem : js.frames) {
			UITexture t;
			t.height = imageItem.frame.h;
			t.width = imageItem.frame.w;
			t.layer = m_freeControlLayer;
			t.uv[0][0] = (float)x / (float)UITextureSize;
			t.uv[0][1] = (float)y / (float)UITextureSize;
			t.uv[2][0] = (float)(x + imageItem.frame.w) / (float)UITextureSize;
			t.uv[2][1] = (float)(y + imageItem.frame.h) / (float)UITextureSize;
			t.uv[1][0] = t.uv[0][0];
			t.uv[1][1] = t.uv[2][1];
			t.uv[3][0] = t.uv[2][0];
			t.uv[3][1] = t.uv[0][1];
			table[imageItem.filename] = t;
		}
		++m_freeControlLayer;
		//
		return true;
	}

	bool UITextureManager::getTexture(const std::string& _filename, const UITexture** _handle)
	{
		for (auto& table : m_imageTables) {
			auto it = table.find(_filename);
			if (it != table.end()) {
				*_handle = &it->second;
				return true;
			}
		}
		return false;
	}

}