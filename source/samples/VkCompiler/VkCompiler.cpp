#include <NixApplication.h>
#include <NixRenderer.h>
#include <nix/io/archieve.h>
#include <cstdio>
#include <VkShaderCompiler/VkShaderCompiler.h>


class VkCompiler : public NixApplication {
	virtual bool initialize( void* _wnd, Nix::IArchieve* _arch) {
        printf("%s", "HelloWorld is initializing!");
		//
		HMODULE library = ::LoadLibraryA("VkShaderCompiler.dll");
		if (!library) {
			return false;
		}
		PFN_GETSHADERCOMPILER getShaderCompiler = reinterpret_cast<PFN_GETSHADERCOMPILER>(::GetProcAddress(library, "GetVkShaderCompiler"));
		Nix::IShaderCompiler * compiler = getShaderCompiler();
		Nix::TextReader reader;
		reader.openFile(_arch, "shader/vulkan/heightfield/heightfield.vert");
		bool rst = compiler->compile(Nix::ShaderModuleType::VertexShader, reader.getText());
		const uint32_t* spv;
		size_t nWord;
		rst = compiler->getCompiledSpv(&spv, &nWord);
		//
		compiler->parseSpvLayout(Nix::ShaderModuleType::VertexShader, spv, nWord);
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

VkCompiler theapp;

NixApplication* GetApplication() {
    return &theapp;
}