project( TextRenderer )

include_directories(
	${CMAKE_SOURCE_DIR}/source/common
	${CMAKE_SOURCE_DIR}/source/module/UIRenderer
	${CMAKE_SOURCE_DIR}/source/module/TinyUI
)

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TextRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextRenderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( TextRenderer WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    TextRenderer
	TinyUI
	UIRenderer
    nix
)