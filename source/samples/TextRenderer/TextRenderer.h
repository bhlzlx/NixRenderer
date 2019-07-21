#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <TexturePacker/TexturePacker.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


#include "NixFontTextureManager.h"
#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>
#include <random>
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Nix {

	class Triangle : public NixApplication {
	private:
		IDriver* m_driver;
		IContext* m_context;
		IRenderPass* m_mainRenderPass;
		IGraphicsQueue* m_primQueue;
		//
		IMaterial* m_material;

		IArgument* m_argCommon;
		uint32_t					m_samBase;
		uint32_t					m_matGlobal;

		IArgument* m_argInstance;
		uint32_t					m_matLocal;

		IRenderable* m_renderable;

		IBuffer* m_vertexBuffer;
		IBuffer* m_indexBuffer;

		IPipeline* m_pipeline;

		ITexture* m_texture;
		ITexturePacker* m_texturePacker;
		FontTextureManager m_fontTextureManager;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve);

		virtual void resize(uint32_t _width, uint32_t _height);

		virtual void release();

		virtual void tick();

		virtual const char* title();

		virtual uint32_t rendererType();
	};
}

extern Nix::Triangle theapp;

