#include "NixFontTextureManager.h"
#include <nix/io/archieve.h>
#include <stb_truetype.h>
#include <stb_image.h>
#include "NixUITextureManager.h"

namespace Nix {

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
		bool rst = m_fontTexManager.initialize(_context, _archieve, m_texture, createTexturePacker, _numFontLayer);
		if (!rst) {
			return false;
		}
		//
		m_imageTables.resize(_numControlLayer);
		m_freeControlLayer = _numFontLayer;
	}

	bool UITextureManager::addPackedLayer(const char* _jsonTable, const char* _imageFile)
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
		assert(x == y == UITextureSize);
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
		// json

	}

	bool UITextureManager::getTexture(const std::string& _filename, UITexture* const* _handle)
	{
		return false;
	}

}