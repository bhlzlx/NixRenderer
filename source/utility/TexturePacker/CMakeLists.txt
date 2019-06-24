project( TexturePacker )

add_definitions( "-DNIX_DYNAMIC_LINK" )

set( Source
	TexturePacker.cpp
	TexturePacker.h
)

add_library( TexturePacker SHARED
	${Source}
)