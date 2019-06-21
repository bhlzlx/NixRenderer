project( TexEncoder )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TexEncoder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( TexEncoder WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    TexEncoder
    nix
)