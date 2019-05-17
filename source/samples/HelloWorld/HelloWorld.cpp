#include <NixApplication.h>
#include <NixRenderer.h>
#include <nix/io/io.h>
#include <cstdio>

class HelloWorld : public NixApplication {
	virtual bool initialize( void* _wnd, nix::IArchieve* ) {
        printf("%s", "HelloWorld is initializing!");
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

HelloWorld theapp;

NixApplication* GetApplication() {
    return &theapp;
}