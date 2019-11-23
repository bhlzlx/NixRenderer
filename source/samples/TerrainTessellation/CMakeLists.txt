project( TerrainTessellation )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainTessellation.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../FreeCamera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( TerrainTessellation WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    TerrainTessellation
    nix
)