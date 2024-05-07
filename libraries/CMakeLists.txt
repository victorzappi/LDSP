# CMakeLists.txt in the libraries directory

# Find all source files excluding OrtModel, which will be dealt with separately
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS *.cpp *.c)
file(GLOB_RECURSE ORTMODEL_SOURCES CONFIGURE_DEPENDS OrtModel/*.cpp OrtModel/*.c)

# Exclude OrtModel sources if not building with ADD_ONNX
list(REMOVE_ITEM ALL_SOURCES ${ORTMODEL_SOURCES})

# Add a base library from remaining sources
add_library(libraries OBJECT ${ALL_SOURCES})

# Include directories
target_include_directories(libraries PUBLIC ../include)
target_include_directories(libraries PUBLIC ..)
target_include_directories(libraries PUBLIC ../dependencies/serial/include) # needed by Arduino
target_include_directories(libraries PUBLIC Arduino)
target_include_directories(libraries PUBLIC ../dependencies/libsndfile/include) # needed by AudioFile
target_include_directories(libraries PUBLIC AudioFile)
target_include_directories(libraries PUBLIC Biquad)
target_include_directories(libraries PUBLIC ../dependencies/seasocks/src/main/c) # needed by Gui and core/WSServer
target_include_directories(libraries PUBLIC Gui)
target_include_directories(libraries PUBLIC GuiController)
target_include_directories(libraries PUBLIC Granular)
target_include_directories(libraries PUBLIC JSON)
target_include_directories(libraries PUBLIC LinInterpolator)
target_include_directories(libraries PUBLIC Oscillator)
target_include_directories(libraries PUBLIC OscReceiver)
target_include_directories(libraries PUBLIC OscSender)
target_include_directories(libraries PUBLIC UdpClient)
target_include_directories(libraries PUBLIC UdpServer)
target_include_directories(libraries PUBLIC XML)
target_include_directories(libraries PUBLIC ../dependencies/fftw3/api)  # needed by FFT
target_include_directories(libraries PUBLIC FFT)

# if ADD_ONNX is set, add ONNX-specific sources and include paths
# this check is needed because ONNX is a dynamic lib
if(ADD_ONNX)
  add_library(onnx_library OBJECT ${ORTMODEL_SOURCES})
  # Add the appropriate ONNX include directories based on the device architecture
  if(${DEVICE_ARCH} STREQUAL "aarch64")
    target_include_directories(onnx_library PUBLIC "../dependencies/onnxruntime/include/headers")
  else()
    target_include_directories(onnx_library PUBLIC "../dependencies/onnxruntime/include/onnxruntime/core/session")
  endif()
  target_include_directories(onnx_library PUBLIC OrtModel)
endif()
