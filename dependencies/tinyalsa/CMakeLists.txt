# CMakeLists.txt designed for LDSP dependency

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c)

add_library(tinyalsa STATIC ${SOURCES})

target_include_directories(tinyalsa PUBLIC include)
