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
#ifdef _WIN32
    #include <Windows.h>
#endif
#include <chrono>

// this demo does not rendering any thing!
// only connect to `PhysX Visual Debugger` for test

namespace Nix {
    
	class PhysXCCT : public NixApplication {
	private:
		std::chrono::time_point<std::chrono::system_clock> m_timePoint;
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

extern Nix::PhysXCCT theapp;
NixApplication* GetApplication();