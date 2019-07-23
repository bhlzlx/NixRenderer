CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

include_directories( 
	${CMAKE_CURRENT_SOURCE_DIR}
)

project( nix )

set( NIX_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/io/archieve.h
    ${CMAKE_CURRENT_SOURCE_DIR}/io/archieve.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/string/encoding.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/string/path.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/memory/BuddySystemAllocator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/memory/BuddySystemAllocator.cpp
    )

add_library( nix STATIC 
	${NIX_SOURCE}
)