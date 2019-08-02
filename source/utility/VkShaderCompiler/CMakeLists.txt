project( VkShaderCompiler )

add_definitions( "-DNIX_DYNAMIC_LINK" )

set(CMAKE_DEBUG_POSTFIX "")

if(MSVC)
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

    set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    #include(ChooseMSVCCRT.cmake)
endif(MSVC)

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