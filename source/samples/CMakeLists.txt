project( Samples )

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(
	${SOLUTION_DIR}/source/Utility
)

set( EXECUTABLE_TYPE )

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
    set( EXECUTABLE_TYPE WIN32 )
endif()

if( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" )
    set( PLATFORM_SOURCE ${SOLUTION_DIR}/source/platform/Linux/LinuxMain.cpp )
endif()

if( ANDROID )
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -E")
    add_subdirectory( Android )
    SET_PROPERTY(TARGET NixApp PROPERTY FOLDER "Samples")
else()
    if( WIN32 )
	
		add_definitions(-DUNICODE -D_UNICODE)
		
        add_subdirectory( HelloWorld )
        add_subdirectory( ClearScreen )
        add_subdirectory( Triangle )
        add_subdirectory( WavefrontModel )
        add_subdirectory( Sphere )
        add_subdirectory( SaveTexArrayRawData )
        add_subdirectory( HDR )
		add_subdirectory( TexEncoder )
		add_subdirectory( PBR )
        add_subdirectory( BasicShading )
        add_subdirectory( PhysXParticle )

        SET_PROPERTY(TARGET HelloWorld PROPERTY FOLDER "Samples")
        SET_PROPERTY(TARGET ClearScreen PROPERTY FOLDER "Samples")
        SET_PROPERTY(TARGET Triangle PROPERTY FOLDER "Samples")
        SET_PROPERTY(TARGET WavefrontModel PROPERTY FOLDER "Samples")
        SET_PROPERTY(TARGET Sphere PROPERTY FOLDER "Samples")
		SET_PROPERTY(TARGET HDR PROPERTY FOLDER "Samples")
        SET_PROPERTY(TARGET TexEncoder PROPERTY FOLDER "Samples")
		SET_PROPERTY(TARGET PBR PROPERTY FOLDER "Samples")
		SET_PROPERTY(TARGET BasicShading PROPERTY FOLDER "Samples")
		SET_PROPERTY(TARGET PhysXParticle PROPERTY FOLDER "Samples")
    endif()
endif()

if(  CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" )
    add_subdirectory( ClearScreen )
endif()
