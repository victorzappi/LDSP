# CMakeLists.txt designed for LDSP dependency

# glob is included in bionic (Android's C++ runtime) only since Android 9 (API 28)
# Referenced at: https://github.com/android/ndk/issues/718
# Android glob source that we use: https://cs.android.com/android/platform/superproject/+/master:bionic/libc/upstream-freebsd/lib/libc/gen/glob.c

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USE_BSD=1")
add_library(glob STATIC glob.c)

target_include_directories(glob PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})