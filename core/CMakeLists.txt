cmake_minimum_required(VERSION 3.25)

project(ldsp)

# https://stackoverflow.com/a/62085797
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

list(APPEND SOURCES
  ${LDSP_PROJECT}/render.cpp
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
  PRIVATE libraries
  PRIVATE android
)
