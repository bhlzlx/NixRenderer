project( Host2DFFTFilter )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/Host2DFFTFilter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( Host2DFFTFilter WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    Host2DFFTFilter
    nix
	fftw3.lib
)