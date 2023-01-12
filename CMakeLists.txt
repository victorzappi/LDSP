# cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-24 -DANDROID_NDK=$NDK -G Ninja .

cmake_minimum_required(VERSION 3.25)

project(ldsp)

add_subdirectory(core)
add_subdirectory(libraries)
