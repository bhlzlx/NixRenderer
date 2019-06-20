SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory( TexturePacker )
add_subdirectory( VkShaderCompiler )

SET_PROPERTY(TARGET VkShaderCompiler PROPERTY FOLDER "Utility")
SET_PROPERTY(TARGET TexturePacker PROPERTY FOLDER "Utility")