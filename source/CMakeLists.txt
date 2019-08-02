project( SOURCE )

include_directories(
    ${SOLUTION_DIR}/source/common
    ${SOLUTION_DIR}/include
    ${SOLUTION_DIR}/thirdpart
)

add_subdirectory( renderer )
add_subdirectory( utility )
add_subdirectory( samples )
add_subdirectory( module )