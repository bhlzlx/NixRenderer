project( PhysXCCT )

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
	${CMAKE_CURRENT_SOURCE_DIR}/PhysXCCT.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PhysXCCT.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( PhysXCCT WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    PhysXCCT
	PhysX_64
	PhysXCommon_64
	PhysXCooking_64
	PhysXExtensions_static_64
	PhysXFoundation_64
	PhysXPvdSDK_static_64
	PhysXCharacterKinematic_static_64
    nix
)