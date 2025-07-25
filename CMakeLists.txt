cmake_minimum_required(VERSION 3.11)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LDSP project to build")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast") #VIC better than "-O3 -ffast-math"

add_compile_options(-fdiagnostics-color=always)



add_subdirectory(core)
add_subdirectory(dependencies)
add_subdirectory(libraries)
