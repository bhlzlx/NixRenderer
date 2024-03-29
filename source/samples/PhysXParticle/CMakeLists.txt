project( PhysXParticle )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdpart/MemoryPool/C-11
	${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdpart/physx
)

link_directories(
 #E:/Github/PhysX/physx/bin/win.x86_64.vc140.mt/debug
 E:/PhysX/physx/bin/win.x86_64.vc140.mt/debug
)

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/../FreeCamera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXParticle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PhysXParticle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXSystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXScene.h
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXScene.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ParticleManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/ParticleManager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXControllerManager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXControllerManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/Player.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Player.h
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( PhysXParticle WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    PhysXParticle
	PhysX_64
	PhysXCommon_64
	PhysXCooking_64
	PhysXExtensions_static_64
	PhysXFoundation_64
	PhysXPvdSDK_static_64
	PhysXCharacterKinematic_static_64
    nix
)