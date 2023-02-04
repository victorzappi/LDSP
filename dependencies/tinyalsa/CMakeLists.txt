file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

add_library(tinyalsa OBJECT ${SOURCES})

target_include_directories(tinyalsa PUBLIC include)
