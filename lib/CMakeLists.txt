add_library(mapcore STATIC
	src/TileSplitter.cpp
	src/TileIndex.cpp
	src/ViewportAssembler.cpp
	src/stb_image_all.cpp
)

target_include_directories(mapcore PUBLIC include)

set_target_properties(mapcore PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
