project ( TinyUI )

include_directories(
	${CMAKE_SOURCE_DIR}/source/module/UIRenderer
)

include_directories(
    ${CMAKE_SOURCE_DIR}/source/Utility
    ${CMAKE_SOURCE_DIR}/source/common
    ${CMAKE_SOURCE_DIR}/source/module/UIRenderer
)

set( UISource
	#
	${CMAKE_CURRENT_SOURCE_DIR}/NixUISystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/NixUISystem.h
	#
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIWidgetUtils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIWidgetUtils.h
	#
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIWidget.h
	#
	${CMAKE_CURRENT_SOURCE_DIR}/NixUILabel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUILabel.h
	#
	${CMAKE_CURRENT_SOURCE_DIR}/NixUIImage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NixUIImage.h

)

add_library( TinyUI STATIC
    ${UISource}
)