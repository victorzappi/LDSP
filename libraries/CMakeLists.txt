# CMakeLists.txt in libraries directory

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

add_library(libraries OBJECT ${SOURCES})

# general includes
target_include_directories(libraries PUBLIC ../include)
# for libraries to include other libraries
target_include_directories(libraries PUBLIC ..)

target_include_directories(libraries PUBLIC ../dependencies/serial/include) # needed by Arduino
target_include_directories(libraries PUBLIC Arduino)
target_include_directories(libraries PUBLIC ../dependencies/libsndfile/include) # needed by AudioFile
target_include_directories(libraries PUBLIC AudioFile)
target_include_directories(libraries PUBLIC Biquad)
target_include_directories(libraries PUBLIC ../dependencies/seasocks/src/main/c) # needed by Gui
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
# include_directories(../dependencies/fftw3/api)
# link_directories(../dependencies/fftw3)
target_include_directories(libraries PUBLIC FFT)

# target_link_libraries(libraries PUBLIC sndfile)
# target_link_libraries(libraries PUBLIC serial)

