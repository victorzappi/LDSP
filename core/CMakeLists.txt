# CMakeLists.txt in core directory


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


target_include_directories(ldsp
  PRIVATE ../include
  PRIVATE ..
)


# Check if we should add libpd
if(ADD_LIBPD)
    # All necessary headers for libpd
    set(LIBPD_DIR ../dependencies/libpd)
    set(LIBPD_INCLUDE_DIRS 
      ${LIBPD_DIR}/libpd_wrapper 
      ${LIBPD_DIR}/libpd_wrapper/util 
      ${LIBPD_DIR}/pure-data/src 
      ${LIBPD_DIR}/cpp
    )
    
    # Adding libpd to the target include directories
    target_include_directories(ldsp
      PRIVATE ${LIBPD_INCLUDE_DIRS}
    )

    # Linking libpd static library
    target_link_libraries(ldsp
      PRIVATE libpd_static
    )
endif()

# Check if we should add fftw3
if(ADD_FFTW3)
  target_link_libraries(ldsp
    PRIVATE fftw3f
  )
endif()

# Check if we should add seasocks
if(ADD_SEASOCKS)
  target_link_libraries(ldsp
    PRIVATE seasocks
  )
endif()


# These are mandatory/lightweight
target_link_libraries(ldsp
  PRIVATE libraries
  PRIVATE android
  PRIVATE tinyalsa
  PRIVATE sndfile 
  PRIVATE glob
  PRIVATE serial
)

