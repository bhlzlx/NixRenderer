#include <SDL.h>
#include <SDL_syswm.h>
#include <regex>
#include <NixApplication.h>
#include <nix/io/archieve.h>
#include <cassert>

#ifdef _WIN32
    typedef HWND NativeWindow;
#else
    #ifdef __ANDROID__
    #endif
#endif

//IApplication* delegate = nullptr;
bool SDLQuit = false;

SDL_Window* window = nullptr;
NixApplication* object = nullptr;

void SDL_EventProc(const SDL_Event& _event);

extern "C" int main(int argc, char** argv)
{
	object = GetApplication();
	SDL_Event event;
	SDL_Init(SDL_INIT_VIDEO);
#ifdef __ANDROID__
	std::string assetRoot = SDL_AndroidGetExternalStoragePath();
#else
	const char * basePath = SDL_GetBasePath();
	std::string assetRoot = basePath;
    assetRoot.append("/../");
#endif
	// uint32_t rendererType = object->rendererType();

	// uint32_t windowFlag = SDL_WINDOW_RESIZABLE;
	// switch( dt ){
	// 	case nix::Vulkan:
	// 		windowFlag = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
	// 		break;
	// 	case nix::GLES3:
	// 		windowFlag = SDL_WINDOW_RESIZABLE;
	// 		break;
	// 	case nix::DX12:
	// 		windowFlag = SDL_WINDOW_RESIZABLE;
	// 		break;
	// }
	window = SDL_CreateWindow(object->title(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_RESIZABLE);
	nix::IArchieve* arch = nix::CreateStdArchieve(assetRoot);
	struct SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (-1 == SDL_GetWindowWMInfo(window, &wmInfo)) {
		return -1;
	}
	assert(wmInfo.subsystem == SDL_SYSWM_WINDOWS);
	///eglContext.nativeWindow = wmInfo.info.win.window;
	//eglContext.nativeDisplay = wmInfo.info.win.hdc;
	object->initialize((void*)wmInfo.info.win.window,arch);
	object->resize(640, 480);
	//
	while (!SDLQuit)
	{
		while (SDL_PollEvent(&event))
		{
			SDL_EventProc(event);
		}
		object->tick();
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

NixApplication::eMouseButton mouseButton = NixApplication::MouseButtonNone;

void SDL_EventProc(const SDL_Event& _event)
{
	switch (_event.type)
	{
	case SDL_QUIT:
	{
		SDLQuit = true;
		break;
	}
	case SDL_WINDOWEVENT:
	{
		switch (_event.window.event)
		{
		case SDL_WINDOWEVENT_SIZE_CHANGED:
		{
			object->resize(_event.window.data1, _event.window.data2);
			break;
		}
		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_HIDDEN:
		{
			//object->resize(0,0);
			break;
		}
		case SDL_WINDOWEVENT_RESTORED:
		{
 			int w, h;
			SDL_GetWindowSize(window, &w, &h);
 			object->resize( w, h);
			break;
		}
		/*case SDL_WINDOWEVENT_ENTER:
		{
			int w, h;
			SDL_GetWindowSize(window, &w, &h);
			delegate->resize(w, h);
			break;
		}*/
		}
		break;
	}
	case SDL_KEYDOWN:
	{
		//if (_event.key.repeat) 
		//{
		object->onKeyEvent((unsigned char)_event.key.keysym.sym, NixApplication::eKeyDown);
		//}
		break;
	}
	case SDL_MOUSEBUTTONDOWN:
	{
		if (_event.button.button)
		{
			object->onMouseEvent((NixApplication::eMouseButton)_event.button.button, NixApplication::eMouseEvent::MouseDown, _event.button.x, _event.button.y);
			mouseButton = (NixApplication::eMouseButton)_event.button.button;
		}
		break;
	}
	case SDL_MOUSEBUTTONUP:
	{
		object->onMouseEvent((NixApplication::eMouseButton)_event.button.button, NixApplication::eMouseEvent::MouseUp, _event.button.x, _event.button.y);
		mouseButton = NixApplication::MouseButtonNone;
		break;
	}
	case SDL_MOUSEMOTION:
	{
		if( mouseButton )
			object->onMouseEvent(mouseButton, NixApplication::eMouseEvent::MouseMove, _event.button.x, _event.button.y);
		break;
	}
	}
	return;
}