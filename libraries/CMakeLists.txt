file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

add_library(libraries OBJECT ${SOURCES})

target_include_directories(libraries PUBLIC Bela/Biquad)
target_include_directories(libraries PUBLIC tinyalsa/include)
target_include_directories(libraries PUBLIC XML)
