cmake_minimum_required(VERSION 3.25)

project(ldsp)

set(SOURCES
  commandLineArgs.cpp
  ctrlOutputs.cpp
  hwConfig.cpp
  LDSP.cpp
  main.cpp
  mixer.cpp
  priority_utils.cpp
  render.cpp
  sensors.cpp
  tinyalsaAudio.cpp
  tinyAlsaExtension.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin/)
add_executable(ldsp ${SOURCES})

target_include_directories(ldsp PRIVATE ../include)

target_include_directories(ldsp
  PRIVATE ../libraries
  PRIVATE ..
  PRIVATE ../libraries/tinyalsa/include
)

target_link_libraries(ldsp
  PRIVATE bela_biquad
  PRIVATE tinyalsa
  PRIVATE xml
  PRIVATE android
)
