project ( UIRenderer )

include_directories(
    ${SOLUTION_DIR}/source/Utility
    ${SOLUTION_DIR}/source/common
)

set( UIRendererSource 
    # main
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIRenderer.h
    # 
    ${CMAKE_CURRENT_SOURCE_DIR}/NixTextBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixTextBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIDefine.h
    # resource management
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIDrawDataHeap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIDrawDataHeap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIMeshBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIMeshBuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUITextureManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUITextureManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NixFontTextureManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixFontTextureManager.h
    # utils
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIUtils.h
)

add_library( UIRenderer STATIC 
    ${UIRendererSource}
)