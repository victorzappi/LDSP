cmake_minimum_required(VERSION 3.11)

# Ensure that option() honors variables set in the cache
cmake_policy(SET CMP0077 NEW)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LDSP project to build")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -DNDEBUG -flto") #VIC -Ofast is better than "-O3 -ffast-math", -flto is link-time optimization

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_compile_options(-fdiagnostics-color=always)

add_subdirectory(core)
add_subdirectory(dependencies)
add_subdirectory(libraries)
