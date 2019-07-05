#include <NixRenderer.h>
#include <mutex>

extern "C" {

	NIX_API_DECL bool InitializeShaderCompiler();
	NIX_API_DECL bool CompileGLSL2SPV( Nix::ShaderModuleType _type, const char * _text, const uint32_t** _spvBytes, size_t* _length, const char ** _compileLog);
	NIX_API_DECL void FinalizeShaderCompiler();

	typedef bool(*PFN_INITIALIZE_SHADER_COMPILER)();
	typedef bool(*PFN_COMPILE_GLSL_2_SPV)(Nix::ShaderModuleType _type, const char * _text, const uint32_t** _spvBytes, size_t* _length, const char** _compileLog);
	typedef void(*PFN_FINALIZE_SHADER_COMPILER)();

}