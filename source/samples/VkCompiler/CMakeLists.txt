project( VkCompiler )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/VkCompiler.cpp
)

add_executable( VkCompiler WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    VkCompiler
    nix
)