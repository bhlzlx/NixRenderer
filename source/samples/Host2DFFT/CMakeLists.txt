project( Host2DFFT )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/Host2DFFT.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( Host2DFFT WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    Host2DFFT
    nix
	fftw3.lib
)