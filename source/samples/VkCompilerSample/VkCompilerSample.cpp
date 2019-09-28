#include <NixApplication.h>
#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>
#include <VkShaderCompiler/VkShaderCompiler.h>

Nix::IShaderCompiler* compiler = nullptr;

class HelloWorld : public NixApplication {
	virtual bool initialize( void* _wnd, Nix::IArchieve* _arch ) {
        printf("%s", "HelloWorld is initializing!");
		//
		HMODULE library = ::LoadLibraryA("VkShaderCompiler.dll");
		if (!library) {
			return false;
		}
		typedef Nix::IShaderCompiler*(*PFN_GETVKSHADERCOMPILER)();
		PFN_GETVKSHADERCOMPILER getShaderCompiler = reinterpret_cast<PFN_GETVKSHADERCOMPILER>(::GetProcAddress(library, "GetVkShaderCompiler"));
		compiler = getShaderCompiler();
		Nix::TextReader reader;
		bool rst = reader.openFile(_arch, "shader/vulkan/test/test.vert");
		compiler->compile(Nix::ShaderModuleType::VertexShader, reader.getText());
		const uint32_t* spvData = nullptr;
		size_t nWord;
		compiler->getCompiledSpv(spvData, nWord);
		compiler->parseSpvLayout(Nix::ShaderModuleType::VertexShader, spvData, nWord);
		//
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