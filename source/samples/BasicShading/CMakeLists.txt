project( BasicShading )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/../FreeCamera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BasicShading.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdpart/tiny_obj_loader.cc
)

add_executable( BasicShading WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    BasicShading
    nix
)