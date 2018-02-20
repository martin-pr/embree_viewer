project (naturalmotion)

cmake_minimum_required (VERSION 2.8.4)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-std=c++11 -Werror -Wall -Wextra)

# find SDL
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)

# find embree3
find_path(EMBREE_INCLUDE_DIR
	NAMES rtcore.h
	PATHS /usr/local/include/embree3 /usr/include/embree3
)

find_library(EMBREE_LIBRARY
	NAMES embree3
	PATHS /usr/local/lib /usr/lib
)

add_subdirectory (src build)
