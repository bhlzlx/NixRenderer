#include <NixRenderer.h>
#include <mutex>

extern "C" {

	NIX_API_DECL bool InitializeShaderCompiler();
	NIX_API_DECL bool CompileGLSL2SPV( Nix::ShaderModuleType _type, const char * _text, std::vector<uint32_t>& _spvBytes, std::string& _compileLog);
	NIX_API_DECL void FinalizeShaderCompiler();

	typedef bool(*PFN_INITIALIZE_SHADER_COMPILER)();
	typedef bool(*PFN_COMPILE_GLSL_2_SPV)(Nix::ShaderModuleType _type, const char * _text, std::vector<uint32_t>& _spvBytes, std::string& _compileLog);
	typedef void(*PFN_FINALIZE_SHADER_COMPILER)();

}