project( TextAPITest )

include_directories(
	${CMAKE_SOURCE_DIR}/source/common
	${CMAKE_SOURCE_DIR}/source/module/UIRenderer
)

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TextAPITest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextAPITest.h
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( TextAPITest WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    TextAPITest
	UIRenderer
    nix
)