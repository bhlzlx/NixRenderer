project( TextRenderer )

include_directories(
	${PROJECT_SOURCE_DIR}/source/common
)

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
	
    ${CMAKE_CURRENT_SOURCE_DIR}/TextRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextRenderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/NixFonttextureManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixFonttextureManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIDrawDataHeap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIDrawDataHeap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIMeshBuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIMeshBuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIDefine.h
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIUtils.h
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIUtils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIRenderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIRenderer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/NixTextBuilder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/NixTextBuilder.h
	${CMAKE_CURRENT_SOURCE_DIR}/../../common/NixJpImp.cpp
)

add_executable( TextRenderer WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    TextRenderer
    nix
)