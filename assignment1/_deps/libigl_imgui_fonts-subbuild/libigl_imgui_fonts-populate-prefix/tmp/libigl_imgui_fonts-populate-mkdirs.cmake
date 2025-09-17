# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-src")
  file(MAKE_DIRECTORY "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-src")
endif()
file(MAKE_DIRECTORY
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-build"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-subbuild/libigl_imgui_fonts-populate-prefix"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-subbuild/libigl_imgui_fonts-populate-prefix/tmp"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-subbuild/libigl_imgui_fonts-populate-prefix/src/libigl_imgui_fonts-populate-stamp"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-subbuild/libigl_imgui_fonts-populate-prefix/src"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-subbuild/libigl_imgui_fonts-populate-prefix/src/libigl_imgui_fonts-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-subbuild/libigl_imgui_fonts-populate-prefix/src/libigl_imgui_fonts-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl_imgui_fonts-subbuild/libigl_imgui_fonts-populate-prefix/src/libigl_imgui_fonts-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
