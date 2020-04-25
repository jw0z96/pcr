cmake_minimum_required(VERSION 3.10)

# set the project name
project(PointCloudRendering)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# executable
add_executable(PointCloudRendering src/main.cpp)

# include everything under 'include'
include_directories(include)

# find SDL2 libraries from the system
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
message(STATUS "SDL2 includes from ${SDL2_INCLUDE_DIRS}")

# find OpenGL libraries from the system
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
message(STATUS "GLEW includes from ${GLEW_INCLUDE_DIRS}")

# include everything under 'libs', which contains libraries supplied by the project
include_directories(libs)

# link our executable against external libraries
target_link_libraries(PointCloudRendering ${SDL2_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARY})
