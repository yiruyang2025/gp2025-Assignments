cmake_minimum_required(VERSION 3.5...3.28)
include(FetchContent)
project(assignment1)

FetchContent_Declare(
    libigl
    GIT_REPOSITORY https://github.com/libigl/libigl.git
    GIT_TAG v2.5.0
)
set(LIBIGL_GLFW ON)
set(LIBIGL_OPENGL ON)
set(LIBIGL_IMGUI ON)
FetchContent_MakeAvailable(libigl)

if (NOT TARGET viewer_proxy)
    add_library(viewer_proxy STATIC ${CMAKE_CURRENT_LIST_DIR}/../viewer_proxy/viewer_proxy.cpp)
    target_link_libraries(viewer_proxy igl::core igl::imgui igl::glfw)
    set_target_properties(viewer_proxy PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                 ${CMAKE_CURRENT_LIST_DIR}/../viewer_proxy)
endif()

# Add your project files
FILE(GLOB SRCFILES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SRCFILES})
target_link_libraries(${PROJECT_NAME} igl::core igl::imgui igl::glfw viewer_proxy)
