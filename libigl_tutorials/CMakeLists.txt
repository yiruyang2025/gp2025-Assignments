cmake_minimum_required(VERSION 3.5...3.28)
project(libigl_tutorials)
set(LIBIGL_GLFW ON)
set(LIBIGL_OPENGL ON)
set(LIBIGL_IMGUI ON)
set(LIBIGL_BUILD_TUTORIALS ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libigl/CMakeLists.txt")
execute_process(
  COMMAND git clone https://github.com/libigl/libigl.git ${CMAKE_CURRENT_SOURCE_DIR}/libigl)
endif()

add_subdirectory(libigl)