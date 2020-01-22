file(GLOB sources
	*.cpp
)

include_directories(embree_viewer
	${SDL2_INCLUDE_DIRS}
	${SDL2_IMAGE_INCLUDE_DIRS}
	${EMBREE_INCLUDE_DIR}
	${ALEMBIC_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
	${OPENEXR_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIR}/OpenEXR
)

add_executable(embree_viewer ${sources})

target_link_libraries(embree_viewer
	${Boost_LIBRARIES}
	SDL2
	embree3 #${EMBREE_LIBRARY}
	${OPENEXR_LIBRARIES} ${ILMBASE_LIBRARIES}
	AlembicAbc AlembicAbcGeom AlembicAbcCoreAbstract AlembicAbcCoreFactory #${ALEMBIC_LIBRARY}
	tbb
)

install(TARGETS embree_viewer DESTINATION bin)
