project( Modules )

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory( UIRenderer )
add_subdirectory( TinyUI )

SET_PROPERTY(TARGET UIRenderer PROPERTY FOLDER "Modules")
SET_PROPERTY(TARGET TinyUI PROPERTY FOLDER "Modules")
