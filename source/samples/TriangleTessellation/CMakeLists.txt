project( TriangleTessellation )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TriangleTessellation.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( TriangleTessellation WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    TriangleTessellation
    nix
)