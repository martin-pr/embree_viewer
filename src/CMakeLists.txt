file(GLOB sources
	*.cpp
)

find_package(SDL REQUIRED)
include_directories(embree_viewer ${SDL_INCLUDE_DIRS})

find_package(SDL_image REQUIRED)
include_directories(embree_viewer ${SDL_IMAGE_INCLUDE_DIRS})

add_executable(embree_viewer ${sources})
target_link_libraries(embree_viewer ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY})
