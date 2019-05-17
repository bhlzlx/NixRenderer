project( HelloWorld )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld.cpp
)

add_executable( HelloWorld WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    HelloWorld
    SDL2
    SDL2Main
    nix
)