project( VkShaderCompiler )

add_definitions( "-DNIX_DYNAMIC_LINK" )

set(CMAKE_DEBUG_POSTFIX "")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../common
	${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdpart/GLSLang
)

set( Source
	VkShaderCompiler.cpp
	VkShaderCompiler.h
)

add_library( VkShaderCompiler SHARED
	${Source}
)

target_link_libraries( VkShaderCompiler
	glslang
	SPIRV
)