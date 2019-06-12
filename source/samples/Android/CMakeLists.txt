project( NixApp )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/AndroidMain.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_library( NixApp SHARED
    ${PROJECT_SOURCE}
)

target_link_libraries(
    NixApp
    android
    log
    nix
)