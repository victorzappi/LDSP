cmake_minimum_required(VERSION 3.25)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LineageDSP project to build")

add_subdirectory(core)
add_subdirectory(libraries)
