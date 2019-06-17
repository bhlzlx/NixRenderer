project( SaveTexArrayRawData )

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/SaveTexArrayRawData.cpp
)

add_executable( SaveTexArrayRawData WIN32
    ${PROJECT_SOURCE}
)

target_link_libraries(
    SaveTexArrayRawData
    nix
)