project( AndroidSample )

add_definitions( -D_DEBUG )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdpart/MemoryPool/C-11
	${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdpart/physx
)

link_directories(
    E:/PhysX/physx/bin/android.arm.fp-soft/release
)

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/JNIBridge.cpp
    ##
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXParticle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXParticle.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXSystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXScene.h
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXScene.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/ParticleManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/ParticleManager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXControllerManager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/PhysXControllerManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/Player.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../PhysXParticle/Player.h
    ## 
    ${CMAKE_CURRENT_SOURCE_DIR}/../FreeCamera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_library( AndroidSample SHARED
    ${PROJECT_SOURCE}
)

target_link_libraries(
    AndroidSample
    android
    log
    nix
    PhysXCharacterKinematic_static
    PhysXPvdSDK_static
	PhysXCooking_static
    PhysX_static
	PhysXCommon_static
	PhysXExtensions
	PhysXFoundation_static
)