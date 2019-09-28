project( VkCompilerSample )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/VkCompilerSample.cpp
)

add_executable( VkCompilerSample WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    VkCompilerSample
    nix
)