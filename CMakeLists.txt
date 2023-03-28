cmake_minimum_required(VERSION 3.11)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LineageDSP project to build")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math")

if (EXPLICIT_ARM_NEON AND ANDROID_ARM_NEON)
  add_compile_options(-mfpu=neon-fp16)
endif()

add_compile_options(-fdiagnostics-color=always)

# set preprocessor directive: #define ANDORID_API API 
add_compile_definitions(ANDROID_API=${API})

add_subdirectory(core)
add_subdirectory(libraries)
add_subdirectory(dependencies)
