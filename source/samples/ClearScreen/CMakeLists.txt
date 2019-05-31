project( ClearScreen )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/ClearScreen.cpp
)

add_executable( ClearScreen WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    ClearScreen
    SDL2
    SDL2Main
    nix
)