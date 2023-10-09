file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cc *.c)

add_library(serial OBJECT ${SOURCES})

# Include directories for serial
target_include_directories(serial PUBLIC include ../glob)
