# CMakeLists.txt in dependencies directory

add_library(dependencies INTERFACE)

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

# all dependencies' includes are exposed as an INTERFACE target, so that they can be used by core and libraries targets too

#-------
# TINYALSA
# right now, we are using a stripped down [and slightly modified] verion of the tinyalsa source
# used in Lineage OS 14.1: https://github.com/LineageOS/android_external_tinyalsa/tree/cm-14.1
# up-to-date tinyalsa: https://github.com/tinyalsa/tinyalsa/tree/master
# more info in core/tinyalsaAudio.cpp
add_subdirectory(tinyalsa)
target_link_libraries(dependencies INTERFACE tinyalsa)
# tinyalsa does not need to explicitly add includes with:
# target_include_directories()
# because its CMakelists.txt file propagates them to any target linking against the lib with:
# target_include_directories(tinyalsa PUBLIC include)
#-------


#-------
# LIBSNDFILE
# Configuration for libsndfile's provided CMakeLists.txt
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "Enable CPack" FORCE)
set(ENABLE_PACKAGE_CONFIG OFF CACHE BOOL "Generate and install package config file" FORCE)
set(INSTALL_PKGCONFIG_MODULE OFF CACHE BOOL "Generate and install pkg-config module" FORCE)
add_subdirectory(libsndfile)
target_include_directories(dependencies INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libsndfile/include)
target_link_libraries(dependencies INTERFACE sndfile)
#-------


#VIC why not a submodule?
#VIC needs to go before glob
if(ADD_FFTW3)
    # BUILD_SHARED_LIBS option will propogate to fftw3 as well
    set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    set(ENABLE_FLOAT ON CACHE BOOL "single-precision" FORCE)
    add_subdirectory(fftw3)
    target_include_directories(dependencies INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fftw3/api)
    target_link_libraries(dependencies INTERFACE fftw3f)
endif()


if(ADD_SEASOCKS)
    # We are using a custom version of seasocks that builds for Android
    # seasocks source: https://github.com/mattgodbolt/seasocks
    # Set the options for Seasocks
    set(SEASOCKS_SHARED OFF CACHE BOOL "Build seasocks as a shared library" FORCE)
    set(UNITTESTS OFF CACHE BOOL "Build unittests." FORCE)
    set(COVERAGE OFF CACHE BOOL "Build with code coverage enabled" FORCE)
    set(SEASOCKS_EXAMPLE_APP OFF CACHE BOOL "Build the example applications." FORCE)
    set(DEFLATE_SUPPORT OFF CACHE BOOL "Include support for deflate (requires zlib)." FORCE)
    add_subdirectory(seasocks)
    target_include_directories(dependencies INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/seasocks/src/main/c)
    target_link_libraries(dependencies INTERFACE seasocks)
endif()


#-------
# GLOB
# We are using a custom version of glob that builds for old Android
# android glob source: https://cs.android.com/android/platform/superproject/+/master:bionic/libc/upstream-freebsd/lib/libc/gen/glob.c

# include_directories(BEFORE ./glob)
# add_subdirectory(glob)
# link_libraries(glob)
add_subdirectory(glob)
target_link_libraries(dependencies INTERFACE glob)
# glob does not need to explicitly add includes with:
# target_include_directories()
# because we made a CMakelists.txt file inside it that  propagates them to any target linking against the lib with:
# target_include_directories(glob PUBLIC include)
#-------


#-------
# SERIAL
# We are using a custom version of this serial lib: https://github.com/Christian-Prather/youtube-serail-tutorial/tree/main
# that is in turn a stripped down version of this one: https://github.com/wjwwood/serial/tree/main
add_subdirectory(serial)
target_link_libraries(dependencies INTERFACE serial)
# serial does not need to explicitly add includes with:
# target_include_directories()
# because we made a CMakelists.txt file inside it that  propagates them to any target linking against the lib with:
# target_include_directories(serial PUBLIC include)
#-------


if(ADD_LIBPD)
    add_subdirectory(libpd)
    #VIC we get some warnings from libpd and we cannot do anything about them
    # this line hides some of them [not sure why not all of them]
    target_compile_options(libpd PRIVATE "-Wno-everything")
    # All necessary headers for libpd
    set(LIBPD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libpd)
    set(LIBPD_INCLUDE_DIRS 
        ${LIBPD_DIR}/libpd_wrapper 
        ${LIBPD_DIR}/libpd_wrapper/util 
        ${LIBPD_DIR}/pure-data/src 
        ${LIBPD_DIR}/cpp
    )
    target_include_directories(dependencies INTERFACE ${LIBPD_INCLUDE_DIRS})
    target_link_libraries(dependencies INTERFACE libpd_static)
endif()


if(ADD_NE10)
    # for 32 bit targets, ne10 uses 'armv7' instead of 'armv7a'
    set(NE10_ANDROID_TARGET_ARCH "${DEVICE_ARCH}")
    if(${NE10_ANDROID_TARGET_ARCH} STREQUAL "armv7a")
        set(NE10_ANDROID_TARGET_ARCH "armv7")
        set(NE10_ARM_HARD_FLOAT OFF CACHE BOOL " Build Ne10 for hard float abi" FORCE)
    endif()

    # extract API level numbers from ANDROID_PLATFORM string and explicitly set NE10_API_LEVEL to match our phone API_LEVEL
    string(REGEX MATCH "[0-9]+" NE10_API_LEVEL "${ANDROID_PLATFORM}")
    set(ANDROID_API_LEVEL ${NE10_API_LEVEL} CACHE STRING "Ne10 Android API level" FORCE)

    set(NE10_BUILD_SHARED OFF CACHE BOOL "Turn off Build shared library, build as static" FORCE)
    set(NE10_BUILD_EXAMPLES OFF CACHE BOOL "Turn off Build examples" FORCE)
    set(NE10_BUILD_UNIT_TEST OFF CACHE BOOL "Turn off build unit test" FORCE)
    add_subdirectory(Ne10)

    target_include_directories(dependencies INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Ne10/inc)
    target_link_libraries(dependencies INTERFACE NE10)
    
    if(ADD_FFT AND ${NE10_FFT} STREQUAL "ON")
      target_compile_definitions(dependencies INTERFACE NE10_FFT="ON")
    endif()
endif()


if(ADD_ONNX)
    add_subdirectory(onnxruntime)
    target_include_directories(dependencies INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/onnxruntime/include/headers)
    target_link_libraries(dependencies INTERFACE onnxruntime)    
endif()


if(ADD_RTNEURAL)
    # Set the CPM source cache to a custom directory within the build directory and create the dir
    # needed because rtneural uses the CMake Package Manager which needs to be explicitly 
    # re-directed to the custom build dir
    set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm_cache")
    file(MAKE_DIRECTORY ${CPM_SOURCE_CACHE})
    add_subdirectory(RTNeural)
    # rtneural does not need to explicitly add includes with:
    # target_include_directories()
    # because its CMakelists.txt file propagates them to any target linking against the lib with:
    # target_include_directories(RTNeural PUBLIC include)

    #VIC why no target_link_libraries???
endif()


if(ADD_MATHNEON)
    # We are using a custom version of the original math-neon: https://github.com/andrepuschmann/math-neon/tree/master
    # this lib is compatible with armv7a only (aarch64 compilation should achieve same performance), which supports soft-float only on Android!
    # or version of the lib is edited quite a bit:
    # - added return statement to all sfp math functions
    # - hid hfp implementation of fabs [the asm was not recognize as corect]
    # - added extern "C" guard in main inlcude so that C++ compiler looks for the unmangled names
    add_subdirectory(math-neon)
    target_link_libraries(dependencies INTERFACE mathneon)
    # mathneon does not need to explicitly add includes with:
    # target_include_directories()
    # because its CMakelists.txt file propagates them to any target linking against the lib with:
    # target_include_directories(mathneon PUBLIC include)
endif()



