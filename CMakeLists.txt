cmake_minimum_required(VERSION 3.11)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LineageDSP project to build")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math")

add_compile_options(-fdiagnostics-color=always)

add_subdirectory(core)
add_subdirectory(libraries)
