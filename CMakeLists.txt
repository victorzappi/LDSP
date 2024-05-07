cmake_minimum_required(VERSION 3.11)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LDSP project to build")

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




# Dynamic dependency build/inlcusion

# Check if this is a libpd project
# and whether or not to add libpd dependency
if (EXISTS ${LDSP_PROJECT}/_main.pd)
  set(ADD_LIBPD TRUE)
else()
  set(ADD_LIBPD FALSE)
endif()

# Check whether or not to add other large dependencies
# Define the variable names and corresponding strings to search for
set(VARIABLE_NAMES "ADD_SEASOCKS" "ADD_FFTW3" "ADD_ONNX")
# designed to work with two strings per dependency, one specific to ldsp and one to the dependency
set(SEARCH_STRINGS "libraries/Gui/Gui.h;seasocks" "fft.h;fftw3" "libraries/OrtModel/OrtModel.h;onnx")
#TODO ADD_SEASOCKS when/if gui gets ever extended to pd, gui will always be included in pd main 
# the main pd file needs to be excluded and an extra check needs to be added:
# if pd and any patch has a 'gui send' or 'gui receive' ---> then ADD_SEASOCKS
# along with a define that removes the seaksocks code from main pd if those objects are not found


# Initialize global cache variables based on VARIABLE_NAMES
foreach(VAR_NAME IN LISTS VARIABLE_NAMES)
    set(${VAR_NAME} FALSE CACHE BOOL "Initially, ${VAR_NAME} is not found" FORCE)
endforeach()
# Include the script that checks for strings
include("${CMAKE_CURRENT_LIST_DIR}/check_dependencies_inclusion.cmake")
# Print the results
# foreach(VAR_NAME IN LISTS VARIABLE_NAMES)
#     message(STATUS "================State of ${VAR_NAME}: ${${VAR_NAME}}")
# endforeach()

# export dependencies configuration to text file
# define the output text file path
set(output_file "${CMAKE_CURRENT_LIST_DIR}/ldsp_dependencies.conf")
# open the text file and write the results
file(WRITE "${output_file}" "# Dependency States\n")
foreach(VAR_NAME IN LISTS VARIABLE_NAMES)
    file(APPEND "${output_file}" "${VAR_NAME}=\"${${VAR_NAME}}\"\n")
endforeach()
file(APPEND "${output_file}" "ADD_LIBPD=\"${ADD_LIBPD}\"\n")




add_subdirectory(core)
add_subdirectory(libraries)
add_subdirectory(dependencies)
