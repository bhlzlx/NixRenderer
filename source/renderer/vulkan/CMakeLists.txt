
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdpart/include )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdpart )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdpart/SPIRV-Cross )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdpart/GLSLang )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../../common )

add_definitions( "-DNIX_DYNAMIC_LINK" )

project( NixVulkan )

set( RendererSource
	${CMAKE_CURRENT_SOURCE_DIR}/DriverVk.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DriverVk.h
	${CMAKE_CURRENT_SOURCE_DIR}/MaterialVk.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MaterialVk.h
	${CMAKE_CURRENT_SOURCE_DIR}/RenderableVk.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/RenderableVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ContextVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ContextVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SwapchainVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SwapchainVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderPassVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderPassVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BufferVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BufferVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TextureVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextureVk.h
    #${CMAKE_CURRENT_SOURCE_DIR}/ViewVk.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/ViewVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PipelineVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PipelineVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QueueVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QueueVk.h
    #${CMAKE_CURRENT_SOURCE_DIR}/ShaderVk.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/ShaderVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DescriptorSetVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DescriptorSetVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ArgumentVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ArgumentVk.h
	${CMAKE_CURRENT_SOURCE_DIR}/UniformVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UniformVk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DeferredDeletor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DeferredDeletor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DebuggerVk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DebuggerVk.h
    # helper
    ${CMAKE_CURRENT_SOURCE_DIR}/vkhelper/helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/vkhelper/helper.cpp
    # glsl compiler
    #${CMAKE_CURRENT_SOURCE_DIR}/compiler/SPIRVReflection.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/compiler/SPIRVReflection.h
    #${CMAKE_CURRENT_SOURCE_DIR}/compiler/ResourceLimits.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/compiler/ResourceLimits.h
    #${CMAKE_CURRENT_SOURCE_DIR}/compiler/GLSLCompiler.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/compiler/GLSLCompiler.h
    #${CMAKE_CURRENT_SOURCE_DIR}/compiler/VEZ.h
	# compress texture format 
	${CMAKE_CURRENT_SOURCE_DIR}/DDSLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DDSLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/KTXLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/KTXLoader.h
    #
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_api.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vkinc.cpp
)

add_library( NixVulkan SHARED
    ${RendererSource}
)

if( ANDROID )
    target_link_libraries(
        NixVulkan
        #OGLCompiler
        SPIRV
        glslang
        OSDependent
        spirv-cross-core
        nix
		log
		android
    )
else()
    target_link_libraries(
        NixVulkan
        #OGLCompiler
        #SPIRV
        #glslang
        #OSDependent
        spirv-cross-core
        nix
        ${vulkan-1}
    )
endif()
