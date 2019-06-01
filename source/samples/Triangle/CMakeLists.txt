project( Triangle )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/Triangle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( Triangle WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    Triangle
    SDL2
    SDL2Main
    nix
)