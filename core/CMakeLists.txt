project(ldsp)

# https://stackoverflow.com/a/62085797
# Source files for the LDSP core itself
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

# Handle main.cpp separately, as LDSP projects may define their own main.cpp
file(GLOB_RECURSE MAIN_SOURCES CONFIGURE_DEPENDS main.cpp)
list(REMOVE_ITEM SOURCES ${MAIN_SOURCES})

# Remove default pd render from sources, in case this is not a PD project 
file(GLOB_RECURSE DEFAULT_PD_SOURCES CONFIGURE_DEPENDS default_render_pd.cpp)
list(REMOVE_ITEM SOURCES ${DEFAULT_PD_SOURCES})

# Source files in the user's project
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS ${LDSP_PROJECT}/*.cpp ${LDSP_PROJECT}/*.c)

list(APPEND SOURCES ${PROJECT_SOURCES})

# Use the LDSP default_render_pd.cpp if the user doesn't provide their own for a PD project
if (NOT EXISTS ${LDSP_PROJECT}/render.cpp AND EXISTS ${LDSP_PROJECT}/_main.pd)
  list(APPEND SOURCES default_render_pd.cpp)
endif()

# Use the LDSP default main.cpp if the user doesn't provide their own
if(NOT EXISTS ${LDSP_PROJECT}/main.cpp AND NOT EXISTS ${LDSP_PROJECT}/main.c)
  list(APPEND SOURCES main.cpp)
endif()

# set preprocessor directives:
# Avoid rebuilding things we don't need to when switching projects,
# By only passing definition main.cpp
GET_FILENAME_COMPONENT(LDSP_PROJECT_NAME ${LDSP_PROJECT} NAME)
set_property(
  SOURCE ${CMAKE_CURRENT_LIST_DIR}/main.cpp 
  PROPERTY COMPILE_DEFINITIONS PROJECT_NAME="${LDSP_PROJECT_NAME}"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin/)
add_executable(ldsp ${SOURCES})

# All necessary headers for libpd
set(LIBPD_DIR ../dependencies/libpd)
set (LIBPD_INCLUDE_DIRS 
  ${LIBPD_DIR}/libpd_wrapper 
  ${LIBPD_DIR}/libpd_wrapper/util 
  ${LIBPD_DIR}/pure-data/src 
  ${LIBPD_DIR}/cpp
)
target_include_directories(ldsp
  PRIVATE ../include
  PRIVATE ..
  PRIVATE ${LIBPD_INCLUDE_DIRS}
)

# All necessary headers for fftw
target_include_directories(ldsp PRIVATE ../dependencies/fftw3/api)
target_link_directories(ldsp PRIVATE ../dependencies/fftw3)

# All neccessary headers for onnxruntime
set(ONNXRUNTIME_PATH ../dependencies/onnxruntime)
target_include_directories(ldsp PRIVATE "${ONNXRUNTIME_PATH}/include/onnxruntime/core/session" PRIVATE)


target_link_directories(ldsp 
  PRIVATE "../dependencies/onnxruntime/${DEVICE_ARCH}"
)


target_link_libraries(ldsp
  PRIVATE libraries
  PRIVATE android
  PRIVATE tinyalsa
  PRIVATE sndfile 
  PRIVATE glob
  PRIVATE libpd_static
  PRIVATE fftw3f

  # DYNAMIC LIBRARY
  PRIVATE onnxruntime
)

