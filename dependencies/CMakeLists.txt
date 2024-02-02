# CMakeLists.txt in dependencies directory

# apply compiler and linker optimizations for dependencies only
# remove unused code
add_compile_options(-ffunction-sections -fdata-sections)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
# strip debug symbols
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
endif()



# right now, we are using a stripped down [and slightly modified] verion of the tinyalsa source
# used in Lineage OS 14.1: https://github.com/LineageOS/android_external_tinyalsa/tree/cm-14.1
# up-to-date tinyalsa: https://github.com/tinyalsa/tinyalsa/tree/master
# more info in core/tinyalsaAudio.cpp
add_subdirectory(tinyalsa)

# Configuration for libsndfile's provided CMakeLists.txt
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "Enable CPack" FORCE)
set(ENABLE_PACKAGE_CONFIG OFF CACHE BOOL "Generate and install package config file" FORCE)
set(INSTALL_PKGCONFIG_MODULE OFF CACHE BOOL "Generate and install pkg-config module" FORCE)
add_subdirectory(libsndfile)

#VIC why not a submodule?
# FFTW3
# BUILD_SHARED_LIBS option will propogate to fftw3 as well
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(ENABLE_FLOAT ON CACHE BOOL "single-precision" FORCE)
add_subdirectory(fftw3)

# seasocks
# We are using a custom version of seasocks that builds for Android
# seasocks source: https://github.com/mattgodbolt/seasocks
# Set the options for Seasocks
set(SEASOCKS_SHARED OFF CACHE BOOL "Build seasocks as a shared library" FORCE)
set(UNITTESTS OFF CACHE BOOL "Build unittests." FORCE)
set(COVERAGE OFF CACHE BOOL "Build with code coverage enabled" FORCE)
set(SEASOCKS_EXAMPLE_APP OFF CACHE BOOL "Build the example applications." FORCE)
set(DEFLATE_SUPPORT OFF CACHE BOOL "Include support for deflate (requires zlib)." FORCE)
add_subdirectory(seasocks)

# We are using a custom version of glob that builds for old Android
# android glob source: https://cs.android.com/android/platform/superproject/+/master:bionic/libc/upstream-freebsd/lib/libc/gen/glob.c
# Add the custom 'glob' directory to the beginning of the include path
include_directories(BEFORE ./glob)

add_subdirectory(glob)
link_libraries(glob)
# We are using a custom version of this serial lib: https://github.com/Christian-Prather/youtube-serail-tutorial/tree/main
# that is in turn a stripped down version of this one: https://github.com/wjwwood/serial/tree/main
add_subdirectory(serial)
add_subdirectory(libpd)
add_subdirectory(RTNeural)

#VIC we get some warnings from libpd and we cannot do anything about them
# this line hides some of them [not sure why not all of them]
target_compile_options(libpd PRIVATE "-Wno-everything")



