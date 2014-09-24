cmake_minimum_required(VERSION 2.8.4)
project(composite)

set(COMPOSITE_SOURCE_FILES composite.cpp)
set(ALPHAMASK_SOURCE_FILES alphamask.cpp)
set(OIIO_LIB "-lOpenImageIO")
set(OPENGL_LIB "-framework Foundation -framework GLUT -framework OpenGL")
set (GCC_COVERAGE_LINK_FLAGS "${OIIO_LIB} ${OPENGL_LIB}")
set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

add_executable(composite ${COMPOSITE_SOURCE_FILES})
add_executable(alphamask ${ALPHAMASK_SOURCE_FILES})