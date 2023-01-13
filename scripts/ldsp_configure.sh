#!/bin/bash
#
# This script configures the build system (using CMake) for the specified phone
# usage:
# export NDK=path/to/android-ndk
# sh ldsp_configure.sh LG "G2 Mini (g2m)" 7.0.0

vendor=$1
model=$2
version_full=$3

# path to hardware config
hw_config="./phones/$vendor/$model/ldsp_hw_config.json"

# target ABI
arch=$(grep 'target architecture' "$hw_config" | cut -d \" -f 4)
if [[ $arch == "armv7a" ]]; then
    abi="armeabi-v7a"
elif [[ $arch == "aarch64" ]]; then
    abi="arm64-v8a"
elif [[ $arch == "x86" ]]; then
    abi="x86"
elif [[ $arch == "x86_64" ]]; then
    abi="x86_64"
else
    echo "Unknown architecture: $arch"
    exit 1
fi

# target Android version
api_version=$($(dirname $0)/ldsp_convert_android_version.sh $version_full)


# support for NEON floating-point unit
neon_setting=$(grep 'supports neon floating point unit' "$hw_config" | cut -d \" -f 4)
if [[ $arch == "armv7a" ]];
then
    if [[ $neon_setting =~ ^(true|True|yes|Yes|1)$ ]];
    then
        neon="-DANDROID_ARM_NEON=ON"
    else
        neon="-DANDROID_ARM_NEON=OFF"
    fi
else
    neon=""
fi


cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-$api_version -DANDROID_NDK=$NDK $neon -G Ninja .
