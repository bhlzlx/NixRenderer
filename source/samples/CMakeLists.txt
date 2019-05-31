project( Samples )
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if( WIN32 )
    if( ${WINDOW_SYSTEM} STREQUAL SDL2 )
        set( PLATFORM_SOURCE
            ${SOLUTION_DIR}/source/platform/Windows/SDL2/SDL2Main.cpp
        )
    elseif( ${WINDOW_SYSTEM} STREQUAL Win32 )
        set( PLATFORM_SOURCE 
            ${SOLUTION_DIR}/source/platform/Windows/WinMain/WinMain.cpp
            ${SOLUTION_DIR}/source/platform/Windows/WinMain/WinMain.rc
        )
    endif()
endif()

if( ANDROID )
endif()

add_subdirectory( HelloWorld )
add_subdirectory( ClearScreen )
add_subdirectory( Triangle )

SET_PROPERTY(TARGET HelloWorld PROPERTY FOLDER "Samples")
SET_PROPERTY(TARGET ClearScreen PROPERTY FOLDER "Samples")
SET_PROPERTY(TARGET Triangle PROPERTY FOLDER "Samples")