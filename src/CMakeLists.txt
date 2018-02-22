file(GLOB sources
	*.cpp
)

include_directories(embree_viewer
	${SDL_INCLUDE_DIRS}
	${SDL_IMAGE_INCLUDE_DIRS}
	${EMBREE_INCLUDE_DIR}
	${ALEMBIC_INCLUDE_DIR}
)

add_executable(embree_viewer ${sources})

target_link_libraries(embree_viewer
	${SDL_LIBRARY}
	${SDL_IMAGE_LIBRARY}

	#${EMBREE_LIBRARY}
	embree3

	${ALEMBIC_LIBRARY}
)
