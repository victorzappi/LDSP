# CMakeLists.txt in the libraries directory

# Find all source files excluding libraries that are not built
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS *.cpp *.c)

# Exclude FFT sources if not building with Fftw3 lib
if(NOT ADD_FFTW3)
  file(GLOB_RECURSE FFT_SOURCES CONFIGURE_DEPENDS Fft/*.cpp Fft/*.c)
  list(REMOVE_ITEM ALL_SOURCES ${FFT_SOURCES})
endif()

# Exclude Gui sources if not building with Seasocks lib
if(NOT ADD_SEASOCKS)
  file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS Gui/*.cpp Gui/*.c)
  file(GLOB_RECURSE GUICONTROLLER_SOURCES CONFIGURE_DEPENDS GuiController/*.cpp GuiController/*.c)
  list(REMOVE_ITEM ALL_SOURCES ${ORTMODEL_SOURCES})
endif()

# Exclude OrtModel sources if not building with Onnx Runtime lib
if(NOT ADD_ONNX)
  file(GLOB_RECURSE ORTMODEL_SOURCES CONFIGURE_DEPENDS OrtModel/*.cpp OrtModel/*.c)
  list(REMOVE_ITEM ALL_SOURCES ${ORTMODEL_SOURCES})
endif()


add_library(libraries OBJECT ${ALL_SOURCES})


# --------------- Include directories ---------------

# include - general
target_include_directories(libraries PUBLIC ../include)
target_include_directories(libraries PUBLIC ..)
target_include_directories(libraries PUBLIC ../dependencies/seasocks/src/main/c) # needed by core/WSServer and Gui


# include - libraries and dependencies needed by libraries
target_include_directories(libraries PUBLIC ../dependencies/serial/include) # needed by Arduino
target_include_directories(libraries PUBLIC Arduino)
target_include_directories(libraries PUBLIC ../dependencies/libsndfile/include) # needed by AudioFile
target_include_directories(libraries PUBLIC AudioFile)
target_include_directories(libraries PUBLIC Biquad)
target_include_directories(libraries PUBLIC Granular)
target_include_directories(libraries PUBLIC JSON)
target_include_directories(libraries PUBLIC LinInterpolator)
target_include_directories(libraries PUBLIC Oscillator)
target_include_directories(libraries PUBLIC OscReceiver)
target_include_directories(libraries PUBLIC OscSender)
target_include_directories(libraries PUBLIC UdpClient)
target_include_directories(libraries PUBLIC UdpServer)
target_include_directories(libraries PUBLIC XML)

if(ADD_FFTW3)
  target_include_directories(libraries PUBLIC ../dependencies/fftw3/api)  # needed by FFT
  target_include_directories(libraries PUBLIC Fft)
endif()

if(ADD_SEASOCKS)
  target_include_directories(libraries PUBLIC Gui)
  target_include_directories(libraries PUBLIC GuiController)
endif()

if(ADD_ONNX)
  target_include_directories(libraries PUBLIC "../dependencies/onnxruntime/include/headers") # needed by OrtModel
  target_include_directories(libraries PUBLIC OrtModel)
endif()


# include - other dependencies 
if(ADD_LIBPD)
    # All necessary headers for libpd
    set(LIBPD_DIR ../dependencies/libpd)
    set(LIBPD_INCLUDE_DIRS 
      ${LIBPD_DIR}/libpd_wrapper 
      ${LIBPD_DIR}/libpd_wrapper/util 
      ${LIBPD_DIR}/pure-data/src 
      ${LIBPD_DIR}/cpp
    )
    target_include_directories(libraries PUBLIC ${LIBPD_INCLUDE_DIRS})
endif()

# rtneural does not need to explicitly add includes with
# target_include_directories()
# because its CMakelists.txt file propagates them to any target linking against the lib
