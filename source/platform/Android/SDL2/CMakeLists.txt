project( main )

#set( AppName '00_Triangle' )
set( AppName 01_Texture )
#set( AppName 02_Framebuffer )
#set( AppName '00_Triangle' )

add_subdirectory( SDL2 )

add_library(
        main SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/vulkan_wrapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Common/FreeCamera.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Common/RendererUtils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Common/${AppName}/delegate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Common/${AppName}/delegate.h
)

target_link_libraries(
        main
        SDL2
        VulkanRenderer
        spirv-cross-core
        clan
        ${log}
        ${android}
)
