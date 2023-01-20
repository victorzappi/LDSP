@echo off

set NDK=C:\Users\%USERNAME%\android-ndk-r25b
set PROJECT=examples\Hello World

cmake -DCMAKE_TOOLCHAIN_FILE=%NDK%/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 "-DANDROID_NDK=%NDK%" "-DLDSP_PROJECT=%PROJECT%" -G Ninja .
