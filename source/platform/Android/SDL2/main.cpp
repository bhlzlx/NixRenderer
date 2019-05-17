#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL/SDL_syswm.h>
//#include "vulkan_wrapper.h"
#include <NixApplication.h>
#include <nix/io/io.h>

NixApplication* vkapp = nullptr;

//#undef main

int main(int argc, char *argv[])
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Init(SDL_INIT_VIDEO);
    if( SDL_Vulkan_LoadLibrary(nullptr) != 0 ){
        exit(2);
    }
    
    window = SDL_CreateWindow("vulkan x android",
                     0, 0, // x,y
                     0, 0, // w,h
    SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI );// flag
    if(!window)
        exit(2);

    vkapp = GetApplication();
    std::string assetRoot = SDL_AndroidGetExternalStoragePath();
    assetRoot.push_back('/');
    auto arch = nix::CreateStdArchieve(assetRoot);
    //vkapp->setRoot(assetRoot);
    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (-1 == SDL_GetWindowWMInfo(window, &wmInfo)) {
        return -1;
    }
    vkapp->initialize( (void*)wmInfo.info.android.window,arch );
    /*Sprite sprite = LoadSprite("texture.bmp", renderer);
    if(sprite.texture == NULL)
        exit(2);
        */

    int w,h;
    SDL_Vulkan_GetDrawableSize(window,&w,&h);
    vkapp->resize(w,h);
    /* Main render loop */
    Uint8 done = 0;
    SDL_Event event;
    while(!done)
    {
        /* Check for events */
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT || event.type == SDL_KEYDOWN || event.type == SDL_FINGERDOWN)
            {
                //done = 1;
            }
            if( event.type == SDL_WINDOWEVENT )
            {
                if(event.window.event == SDL_WINDOWEVENT_ENTER )
                {
                    int w,h;
                    SDL_Vulkan_GetDrawableSize(window,&w,&h);
                    vkapp->resize(w,h);
                }
            }
        }
        vkapp->tick();
        /* Draw a gray background */
       // SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
      //  SDL_RenderClear(renderer);

        //draw(window, renderer, sprite);

        /* Update the screen! */
       // SDL_RenderPresent(renderer);

       // SDL_Delay(10);
    }

    exit(0);
}