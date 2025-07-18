# CMakeLists.txt in core directory


project(ldsp)


# --------------- Prepare dynamic dependency build/inclusion ---------------

# Check if this is a libpd project
# and whether or not to add libpd dependency
if (EXISTS ${LDSP_PROJECT}/_main.pd)
  set(ADD_LIBPD TRUE CACHE BOOL "Enable ADD_LIBPD globally" FORCE)
else()
  set(ADD_LIBPD FALSE CACHE BOOL "Disable ADD_LIBPD globally" FORCE)
endif()

# math-neon lib always enabled for armv7a targets
if(${DEVICE_ARCH} STREQUAL "armv7a")
  set(ADD_MATHNEON TRUE CACHE BOOL "Enable ADD_MATHNEON globally" FORCE)
else()
  set(ADD_MATHNEON FALSE CACHE BOOL "Disable ADD_MATHNEON globally" FORCE)
endif()

# Check whether or not to add other large dependencies
# Define the variable names and corresponding strings to search for
set(VARIABLE_NAMES "ADD_SEASOCKS" "ADD_FFT" "ADD_FFTW3" "ADD_NE10" "ADD_ONNX" "ADD_RTNEURAL")
# designed to work with two strings per dependency, one specific to ldsp and one to the dependency
set(SEARCH_STRINGS "libraries/Gui/Gui.h;seasocks" "Fft.h;Fft.h" "fftw3;fftw3" "NE10;NE10" "libraries/OrtModel/OrtModel.h;onnx" "RTNeural/RTNeural.h;rtneural")
#TODO ADD_SEASOCKS when/if gui gets ever extended to pd, gui will always be included in pd main 
# the main pd file needs to be excluded and an extra check needs to be added:
# if pd and any patch has a 'gui send' or 'gui receive' ---> then ADD_SEASOCKS
# along with a define that removes the seaksocks code from main pd if those objects are not found


# Initialize global cache variables based on VARIABLE_NAMES
foreach(VAR_NAME IN LISTS VARIABLE_NAMES)
    set(${VAR_NAME} FALSE CACHE BOOL "Initially, ${VAR_NAME} is not found" FORCE)
endforeach()
# call the script that checks for strings
include("${CMAKE_CURRENT_SOURCE_DIR}/../check_dependencies_inclusion.cmake")



# --------------- Source files ---------------

# https://stackoverflow.com/a/62085797
# Source files for the LDSP core itself
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

# Handle main.cpp separately, as LDSP projects may define their own main.cpp
file(GLOB_RECURSE MAIN_SOURCES CONFIGURE_DEPENDS main.cpp)
list(REMOVE_ITEM SOURCES ${MAIN_SOURCES})

# Remove default pd render from sources, in case this is not a PD project 
file(GLOB_RECURSE DEFAULT_PD_SOURCES CONFIGURE_DEPENDS default_render_pd.cpp)
list(REMOVE_ITEM SOURCES ${DEFAULT_PD_SOURCES})

# If Seasocks dependency is not in use, exclude core sources that depend on it
if(NOT ADD_SEASOCKS)
  file(GLOB_RECURSE SEASOCKS_SOURCES CONFIGURE_DEPENDS WebServer.cpp WSServer.cpp)
  list(REMOVE_ITEM SOURCES ${SEASOCKS_SOURCES})
endif()

# Source files in the user's project
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS ${LDSP_PROJECT}/*.cpp ${LDSP_PROJECT}/*.c)

list(APPEND SOURCES ${PROJECT_SOURCES})

# Use the LDSP default_render_pd.cpp if the user doesn't provide their own for a PD project
if (ADD_LIBPD AND NOT EXISTS ${LDSP_PROJECT}/render.cpp)
  list(APPEND SOURCES default_render_pd.cpp)
endif()

# Use the LDSP default main.cpp if the user doesn't provide their own
if(NOT EXISTS ${LDSP_PROJECT}/main.cpp AND NOT EXISTS ${LDSP_PROJECT}/main.c)
  list(APPEND SOURCES main.cpp)
endif()

# set preprocessor directives:
# Avoid rebuilding things we don't need to when switching projects,
# By only passing the definition to main.cpp
GET_FILENAME_COMPONENT(LDSP_PROJECT_NAME ${LDSP_PROJECT} NAME)
set_property(
  SOURCE ${CMAKE_CURRENT_LIST_DIR}/main.cpp 
  PROPERTY COMPILE_DEFINITIONS PROJECT_NAME="${LDSP_PROJECT_NAME}"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin/)
add_executable(ldsp ${SOURCES})




# --------------- Include directories ---------------
# we expose core headers via an INTERFACE target, so that they can be used by the libraries target too
add_library(core INTERFACE)
target_include_directories(core INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(ldsp PRIVATE core)




# --------------- Dependencies choice ---------------
# check which dependencies to build and add as libs

# almost all phones are equipped with NEON, but it's always good to check!
if(${NEON_SUPPORTED} STREQUAL "ON") 
  # if cmake was set to enable neon to format audio streams
  if(${NEON_AUDIO_FORMAT} STREQUAL "ON")
    # define NEON_AUDIO_FORMAT macro for the ldsp target (used in core)
    target_compile_definitions(ldsp PRIVATE NEON_AUDIO_FORMAT="ON")
  endif()

  # if fft lib included in the user's code
  if(ADD_FFT) 
    # if cmake was set to enable ne10 fft
    if(${NE10_FFT} STREQUAL "ON")  
      # then use ne10 fft implementation, by making sure we build it!
      set(ADD_NE10 TRUE CACHE BOOL "Enable NE10 globally" FORCE)

    else()  
      # otherwise, stick to fftw implementation, by making sure we build that instead
      set(ADD_FFTW3 TRUE CACHE BOOL "Enable FFTW globally" FORCE)
    endif()
  endif()
elseif(ADD_NE10)
  # if neon not supported, let's make sure that we don't try to build it
  set(ADD_NE10 FALSE CACHE BOOL "Disable NE10 globally" FORCE)
  message(WARNING "Trying to build and use Ne10 lib on a device that is not equipped with NEON!")
endif()


if(ADD_LIBPD)
    target_link_libraries(ldsp PRIVATE libpd_static)
endif()

if(ADD_FFTW3)
  target_link_libraries(ldsp PRIVATE fftw3f)
endif()

if(ADD_NE10)
  target_link_libraries(ldsp PRIVATE NE10)
endif()

if(ADD_SEASOCKS)
  target_link_libraries(ldsp PRIVATE seasocks)
endif()

if(ADD_ONNX)
  # onnxruntime is a dynamic lib and needs a target link directory
  target_link_directories(ldsp 
    PRIVATE "../dependencies/onnxruntime/${DEVICE_ARCH}/${ONNX_VERSION}"
  )
  target_link_libraries(ldsp PRIVATE onnxruntime)
endif()

if(ADD_RTNEURAL)
  target_link_libraries(ldsp PRIVATE RTNeural)
endif()

if(ADD_MATHNEON)
  target_link_libraries(ldsp PRIVATE mathneon)
endif()

target_link_libraries(ldsp
  PRIVATE dependencies
  PRIVATE libraries
  PRIVATE android # this is found in the NDK!
)



# --------------- Print dependencies status ---------------

# note that FFTW or NE10 may have been changed since the first check
# foreach(VAR_NAME IN LISTS VARIABLE_NAMES)
#     message(STATUS "================State of ${VAR_NAME}: ${${VAR_NAME}}")
# endforeach()

# export dependencies configuration to text file
# define the output text file path
set(output_file "${CMAKE_CURRENT_SOURCE_DIR}/../ldsp_dependencies.conf")
# overwrite any existing content with the first line (no comment now)
file(WRITE "${output_file}" "")  # empty initialization
# write all dependency flags
foreach(VAR_NAME IN LISTS VARIABLE_NAMES)
    file(APPEND "${output_file}" "${VAR_NAME}=\"${${VAR_NAME}}\"\n")
endforeach()
# manually add these ones cos they are not in the list
file(APPEND "${output_file}" "ADD_LIBPD=\"${ADD_LIBPD}\"\n")
file(APPEND "${output_file}" "ADD_MATHNEON=\"${ADD_MATHNEON}\"\n")