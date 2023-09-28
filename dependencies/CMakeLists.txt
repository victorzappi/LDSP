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

# FFTW3
# BUILD_SHARED_LIBS option will propogate to fftw3 as well
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(ENABLE_FLOAT ON CACHE BOOL "single-precision" FORCE)
add_subdirectory(fftw3)

# Add the custom 'glob' directory to the beginning of the include path
include_directories(BEFORE ./glob)

add_subdirectory(glob)
link_libraries(glob)
add_subdirectory(libpd)

#VIC we get some warnings from libpd and we cannot do anything about them
# this line hides some of them [not sure why not all of them]
target_compile_options(libpd PRIVATE "-Wno-everything")



