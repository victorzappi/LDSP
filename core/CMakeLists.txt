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


#TODO the following logic has to change
# right now, of we pass --no-neon-audio-format to ldsp.sh/bat scrip, we have the same effect as NEON is not supported by arch
# indeed, NEON_AUDIO_FORMAT is set only if NEON_SUPPORTED is set
# this is not what we want, because like this when we disable NEON_AUDIO_FORMAT we completely lose the ability to use NEON on armv7a [on aarch64 NEON is always available, regardless of NEON_SUPPORTED]
# what we want is to decouple NEON_SUPPORTED and NEON_AUDIO_FORMAT
# this means a completely different define passed from the scripts to the cmake, called NEON_AUDIO
# then, set NEON_AUDIO_FORAMT if both NEON_AUDIO and NEON_SUPPORTED are true
# we can anyway leave in the scripts the machanism that sets NEON_AUDIO to off if NEON_SUPPORTED is false [for armv7a]

# get NEON_SUPPORTED Flag
if (${NEON_SUPPORTED} STREQUAL "ON")
  # # Set NEON_AUDIO_FORMAT preprocessor macro for tinyalsaAudio.cpp
  # set_property(
  #   SOURCE ${CMAKE_CURRENT_LIST_DIR}/tinyalsaAudio.cpp 
  #   PROPERTY COMPILE_DEFINITIONS NEON_AUDIO_FORMAT="ON" 
  #   # In the future, NEON_AUDIO_FORMAT will only be true if NEON_AUDIO_FORMAT and NEON_SUPPORTED flags are true
  # )
  # Set NEON_AUDIO_FORMAT preprocessor macro for tinyalsaAudio.h
  # set_property(
    # SOURCE ${CMAKE_CURRENT_LIST_DIR}/../include/tinyalsaAudio.h
    # PROPERTY COMPILE_DEFINITIONS NEON_AUDIO_FORMAT="ON" 
    # In the future, NEON_AUDIO_FORMAT will only be true if NEON_AUDIO_FORMAT and NEON_SUPPORTED flags are true
  # )

  # We cannot use the above because set_property cannot target header files, only .cpp or .c

  # Include header files - We do not need this line! Are header directories getting included to LDSP elsewhere?
  # target_include_directories(ldsp PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../include)

  # Define NEON_AUDIO_FORMAT macro for the target
  target_compile_definitions(ldsp PRIVATE NEON_AUDIO_FORMAT="ON")
endif()


# Check if we should add libpd
if(ADD_LIBPD)
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

# Check if we should add onnx
if(ADD_ONNX)
  # onnxruntime is a dynamic lib and needs a target link directory
  target_link_directories(ldsp 
    PRIVATE "../dependencies/onnxruntime/${DEVICE_ARCH}/${ONNX_VERSION}"
  )
  target_link_libraries(ldsp
    # DYNAMIC LIBRARY
    PRIVATE onnxruntime

  )
endif()

# Check if we should add rtneural
if(ADD_RTNEURAL)
  target_link_libraries(ldsp
    PRIVATE RTNeural
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
