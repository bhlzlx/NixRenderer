project( PhysXParticle )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/../FreeCamera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PhysXParticle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( PhysXParticle WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    PhysXParticle
    nix
)