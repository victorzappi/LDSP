cmake_minimum_required(VERSION 3.11)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LineageDSP project to build")

add_compile_options(-fdiagnostics-color=always)

add_subdirectory(core)
add_subdirectory(libraries)
