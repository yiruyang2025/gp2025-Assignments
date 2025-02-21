cmake_minimum_required(VERSION 3.16.0)
include(FetchContent)
project(viewer_proxy)

FetchContent_Declare(
  libigl
  GIT_REPOSITORY https://github.com/libigl/libigl.git
  GIT_TAG v2.5.0)
set(LIBIGL_GLFW ON)
set(LIBIGL_OPENGL ON)
set(LIBIGL_IMGUI ON)
FetchContent_MakeAvailable(libigl)

file(GLOB SRCFILES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
add_library(${PROJECT_NAME} STATIC ${SRCFILES})
target_link_libraries(${PROJECT_NAME} igl::core igl::imgui igl::glfw)

set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                 ${CMAKE_CURRENT_LIST_DIR})
