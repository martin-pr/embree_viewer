project (embree_viewer)

cmake_minimum_required (VERSION 2.8.4)

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-std=c++11 -Werror -Wall -Wextra)

find_package(Boost REQUIRED COMPONENTS system filesystem program_options)

find_package(SDL2 REQUIRED)

find_package(embree 3 REQUIRED)

find_package(OpenEXR REQUIRED)
find_package(Alembic REQUIRED)

add_subdirectory (src build)
