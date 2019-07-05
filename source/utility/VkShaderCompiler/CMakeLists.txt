project( VkShaderCompiler )

set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD")
set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd")

add_definitions( "-DNIX_DYNAMIC_LINK" )

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