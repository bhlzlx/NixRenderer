SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory( vulkan )
SET_PROPERTY(TARGET NixVulkan PROPERTY FOLDER "Renderers")