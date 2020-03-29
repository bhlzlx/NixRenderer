project( DX12Test )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloDX12.cpp
)

add_executable( DX12Test WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    DX12Test
    nix
)