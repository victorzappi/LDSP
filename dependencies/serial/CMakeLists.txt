file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cc *.c)

add_library(serial STATIC ${SOURCES})

target_include_directories(serial PUBLIC include)

target_link_libraries(serial PUBLIC glob)
