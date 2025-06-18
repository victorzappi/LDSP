cmake_minimum_required(VERSION 3.11)

project(ldsp)

set(LDSP_PROJECT "core" CACHE FILEPATH "Path to the LDSP project to build")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#TODO pass configuration from ldsp script
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math")

# aarch64 has neon always active
if("${DEVICE_ARCH}" STREQUAL "armv7a" AND "${EXPLICIT_ARM_NEON}" STREQUAL "ON")
  add_compile_options(-mfpu=neon)
  set(NEON_SUPPORTED "ON")
# Add NEON_SUPPORTED flag for all aarch64
elseif("${DEVICE_ARCH}" STREQUAL "aarch64" AND "${EXPLICIT_ARM_NEON}" STREQUAL "ON")
  set(NEON_SUPPORTED "ON")
else()
  set(NEON_SUPPORTED "OFF")
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
set(VARIABLE_NAMES "ADD_SEASOCKS" "ADD_FFTW3" "ADD_ONNX" "ADD_RTNEURAL")
# designed to work with two strings per dependency, one specific to ldsp and one to the dependency
set(SEARCH_STRINGS "libraries/Gui/Gui.h;seasocks" "Fft.h;fftw3" "libraries/OrtModel/OrtModel.h;onnx" "RTNeural/RTNeural.h;rtneural")
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
# overwrite any existing content with the first line (no comment now)
file(WRITE "${output_file}" "")  # empty initialization
# write all dependency flags
foreach(VAR_NAME IN LISTS VARIABLE_NAMES)
    file(APPEND "${output_file}" "${VAR_NAME}=\"${${VAR_NAME}}\"\n")
endforeach()
# manually add this one in case it's not in the list
file(APPEND "${output_file}" "ADD_LIBPD=\"${ADD_LIBPD}\"\n")



# dummy usage for DEVICE_ARCH and ONNX_VERSION, to avoid warnings
# in projects that do not use them
if(DEFINED DEVICE_ARCH)
    set(_dummy_DEVICE_ARCH "${DEVICE_ARCH}")
endif()
if(DEFINED ONNX_VERSION)
    set(_dummy_ONNX_VERSION "${ONNX_VERSION}")
endif()



add_subdirectory(core)
add_subdirectory(dependencies)
add_subdirectory(libraries) # here we have all target include directories too
