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
#pragma once
#include "NixUIRenderer.h"
#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>
#include <random>
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Nix {

	class TextSample : public NixApplication {
	private:
		IDriver*				m_driver;
		IContext*				m_context;
		IRenderPass*			m_mainRenderPass;
		IGraphicsQueue*			m_primQueue;
		Nix::UIRenderer			m_uiRenderer;
		//
		Nix::UIDrawData*		m_drawDataText1;
		Nix::UIDrawData*		m_drawDataText2;
		Nix::UIDrawData*		m_drawDataText3;

		float					m_width;
		float					m_height;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve);

		virtual void resize(uint32_t _width, uint32_t _height);

		virtual void release();

		virtual void tick();

		virtual const char* title();

		virtual uint32_t rendererType();
	};
}

extern Nix::TextSample theapp;

