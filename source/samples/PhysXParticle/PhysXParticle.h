#pragma once
#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>

#include "../FreeCamera.h"

#include "PhysXSystem.h"

#ifdef _WIN32
    #include <Windows.h>
#endif
#include <chrono>

namespace Nix {
    
	class PhysXParticle : public NixApplication {
	private:
		IDriver*					m_driver;
		IContext*					m_context;
		IRenderPass*				m_mainRenderPass;
		IGraphicsQueue*				m_primQueue;

		FreeCamera					m_camera;
		//
		IMaterial*					m_material;

		IArgument*					m_argCommon;
		uint32_t					m_argSlot;

		IRenderable*				m_renderable;

		IBuffer*					m_vertexBuffer;

		IPipeline*					m_pipeline;

		ITexture*					m_texture;

		float wndWidth;
		float wndHeight;

		std::chrono::time_point<std::chrono::system_clock> m_timePoint;

		PhysXSystem*				m_phySystem;
		PhysXScene*					m_phyScene;

		// physx


		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve );

		virtual void resize(uint32_t _width, uint32_t _height);

		virtual void release();

		virtual void tick();

		virtual const char * title();

		virtual uint32_t rendererType();

		virtual void onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y);

		virtual void onKeyEvent(unsigned char _key, eKeyEvent _event);
	};
}

extern Nix::PhysXParticle theapp;
NixApplication* GetApplication();