# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-src")
  file(MAKE_DIRECTORY "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-src")
endif()
file(MAKE_DIRECTORY
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-build"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-subbuild/libigl-populate-prefix"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-subbuild/libigl-populate-prefix/tmp"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-subbuild/libigl-populate-prefix/src/libigl-populate-stamp"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-subbuild/libigl-populate-prefix/src"
  "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-subbuild/libigl-populate-prefix/src/libigl-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-subbuild/libigl-populate-prefix/src/libigl-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/yangyiru/gp2025-Assignments/assignment1/_deps/libigl-subbuild/libigl-populate-prefix/src/libigl-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
