# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.4)
project(NixGraphics)
# if( ANDROID )
# elseif( APPLE )
#     if( IOS )
#     else()
#     endif
# elseif( WIN32 )
# endif

set( WINDOW_SYSTEM Win32 )
#set( WINDOW_SYSTEM SDL2 )
# set build variable && output directory
if( ANDROID )
    # android platform
    set( TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI} )
    if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fdeclspec -g -Wall -Wunused-function -pthread")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fdeclspec -pthread")
        add_link_options("-Wl,--build-id=sha1")
        set(TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI} )
    else()
        message("why not clang!?!?")
    endif()
    # add sdl2 library for Android
    include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/SDL2/include )
    # add basic NDK native library
    find_library( log log )
    find_library( android android )
   # set( CMAKE_FIND_LIBRARY_PREFIXES lib )
   # set( CMAKE_FIND_LIBRARY_SUFFIXES a so )
elseif( APPLE )
    if( IOS )
    # iOS platform
    set( TARGET_ARCH iOS )
    else()
    # macOS platform
    set( TARGET_ARCH macOS )
    endif()
   # set( CMAKE_FIND_LIBRARY_PREFIXES lib )
   # set( CMAKE_FIND_LIBRARY_SUFFIXES a dylib )
elseif( WIN32 )
    # windows platform
    if( DEFINED TARGET_ARCH )
        if( ${TARGET_ARCH} STREQUAL x86 )
        elseif( ${TARGET_ARCH} STREQUAL x64 )
        else()
            set( TARGET_ARCH x64 )
            message("Invalid Win32 Architechture, set architecture to 'x64'...")
        endif()
    else()
        set( TARGET_ARCH x64 )
        message("Architechture was not set, set architecture to 'x64'...")
    endif()
    # MingW Compilers
    if( MINGW )
    if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wunused-function -pthread")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -g -pthread")
    elseif( CMAKE_BUILD_TYPE STREQUAL "Release" )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -pthread -Wunused-function")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -pthread")
    endif()                 
    #
    if( TARGET_ARCH STREQUAL x64 )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    elseif(TARGET_ARCH STREQUAL x86)
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    endif()

    endif()
    # Microsoft Visual C++ Compilers
    if(MSVC)
    if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  /fp:fast /Gy /Oi /Oy /O2 /Ot /Zi /EHsc ")
    endif()
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    IF(CMAKE_CL_64)
        set( TARGET_ARCH "x64" )
    ELSE(CMAKE_CL_64)
        set( TARGET_ARCH "x86" )
    ENDIF(CMAKE_CL_64)
    endif()
endif()

message( "target platform : ${CMAKE_SYSTEM_NAME}")

set( SOLUTION_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

SET( EXECUTABLE_OUTPUT_PATH 
    ${SOLUTION_DIR}/bin/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)
SET( LIBRARY_OUTPUT_PATH 
    ${SOLUTION_DIR}/bin/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG 
    ${SOLUTION_DIR}/bin/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)

link_directories(
    ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)

include_directories(
    ${SOLUTION_DIR}/include
    ${SOLUTION_DIR}/include/SDL
    ${SOLUTION_DIR}/include/OpenGLES
    ${SOLUTION_DIR}/include/Utils
)

if( MSVC )
    set( THIRD_PART_LIB_DIR ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH} )

    find_library( EGL libEGL ${THIRD_PART_LIB_DIR} )
    find_library( GLESv2 libGLESv2 ${THIRD_PART_LIB_DIR} )

    if( ${WINDOW_SYSTEM} STREQUAL SDL2 )
        find_library( SDL2 SDL2 "SDL2.lib" ${THIRD_PART_LIB_DIR} )
    else()
    endif()
endif()

add_subdirectory( thirdpart )
add_subdirectory( source )