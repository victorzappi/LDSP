
# to compile pure-data, we need glob and globfree symbols
# glob is not included in android ndk until version > 9.0 (API Level 28)
# To avoid having to update ndk on phones, we can just include the glob library ourself.
# Referenced at: https://github.com/android/ndk/issues/718

# glob for android taken from ndk distribution:
# https://cs.android.com/android/platform/superproject/+/master:bionic/libc/upstream-freebsd/lib/libc/gen/glob.c

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USE_BSD=1")
add_library(glob STATIC glob.c)

target_include_directories(glob PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})