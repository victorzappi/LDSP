# CMakeLists.txt in the libraries directory

# Find all source files excluding libraries that are not built
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS *.cpp *.c)

# Exclude FFT sources if not building with Fft lib
if(NOT ADD_FFT)
  file(GLOB_RECURSE FFT_SOURCES CONFIGURE_DEPENDS Fft/*.cpp Fft/*.c)
  list(REMOVE_ITEM ALL_SOURCES ${FFT_SOURCES})
endif()

# Exclude Gui sources if not building with Seasocks lib
if(NOT ADD_SEASOCKS)
  file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS Gui/*.cpp Gui/*.c)
  list(REMOVE_ITEM ALL_SOURCES ${GUI_SOURCES})
  file(GLOB_RECURSE GUICONTROLLER_SOURCES CONFIGURE_DEPENDS GuiController/*.cpp GuiController/*.c)
  list(REMOVE_ITEM ALL_SOURCES ${GUICONTROLLER_SOURCES})
endif()

# Exclude OrtModel sources if not building with Onnx Runtime lib
if(NOT ADD_ONNX)
  file(GLOB_RECURSE ORTMODEL_SOURCES CONFIGURE_DEPENDS OrtModel/*.cpp OrtModel/*.c)
  list(REMOVE_ITEM ALL_SOURCES ${ORTMODEL_SOURCES})
endif()


add_library(libraries OBJECT ${ALL_SOURCES})


# --------------- Include directories ---------------

# include - from core via linker, always needed
target_link_libraries(libraries PRIVATE core)

# include - from dependencies via linker, needed by some LDSP libs
target_link_libraries(libraries PRIVATE dependencies)

# include - specific to LDSP libraries
target_include_directories(libraries PUBLIC Arduino)
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

if(ADD_FFT)
  target_include_directories(libraries PUBLIC Fft)
endif()

if(ADD_SEASOCKS)
  target_include_directories(libraries PUBLIC Gui)
  target_include_directories(libraries PUBLIC GuiController)
endif()

if(ADD_ONNX)
  target_include_directories(libraries PUBLIC OrtModel)
endif()