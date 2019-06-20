project( ClearScreen )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/ClearScreen.cpp
)

add_executable( ClearScreen ${EXECUTABLE_TYPE}
    ${PROJECT_SOURCE}
)

if( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" )
target_link_libraries(
    ClearScreen
    dl
    X11
    nix
)
else()
target_link_libraries(
    ClearScreen
    nix
)
endif()

