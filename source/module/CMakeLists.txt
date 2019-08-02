project( Modules )

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory( UIRenderer )

SET_PROPERTY(TARGET UIRenderer PROPERTY FOLDER "Renderer-Modules")
