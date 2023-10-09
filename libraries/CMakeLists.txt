file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

add_library(libraries OBJECT ${SOURCES})

target_include_directories(libraries PUBLIC Biquad)
target_include_directories(libraries PUBLIC Arduino)
target_include_directories(libraries PUBLIC JSON)
target_include_directories(libraries PUBLIC LinInterpolator)
target_include_directories(libraries PUBLIC OscReceiver)
target_include_directories(libraries PUBLIC OscSender)
target_include_directories(libraries PUBLIC UdpClient)
target_include_directories(libraries PUBLIC UdpServer)
target_include_directories(libraries PUBLIC XML)
target_include_directories(libraries PUBLIC ../include)
target_include_directories(libraries PUBLIC ..)
include_directories(../dependencies/fftw3/api)
link_directories(../dependencies/fftw3)
target_include_directories(libraries PUBLIC FFT)

target_link_libraries(libraries PUBLIC sndfile)
target_link_libraries(libraries PUBLIC serial)

