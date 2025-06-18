#!/system/bin/sh
#
# This script retrieves the phone's code name, android version and architecture
# to fill in the hw config file and use ldsp.sh correctly
# usage:
# sh ldsp_phoneDetails.sh


echo "Entries for ldsp_hw_config.json:"

# get model code name
BRAND=$(getprop ro.product.brand)
MODEL=$(getprop ro.product.model)
echo "\tdevice: $BRAND $MODEL"


# get architecture from uname
ARCH=$(uname -m)

# normalize and print architecture
case "$ARCH" in
    aarch64)
        echo '\ttarget architecture: aarch64'
        echo '\tsupports neon floating point unit: true' # always supported on aarch64
        ;;
    armv7l|armv8l)
        echo '\ttarget architecture: armv7a'
        # check NEON support from first Features line
        FEATURES=$(grep -i 'Features' /proc/cpuinfo | head -n 1)
        if echo "$FEATURES" | grep -qw 'asimd'; then
            echo '\tsupports neon floating point unit: true'  # ARMv8 SIMD
        elif echo "$FEATURES" | grep -qw 'neon'; then
            echo '\tsupports neon floating point unit: true'
        else
            echo '\tsupports neon floating point unit: false'
        fi
        ;;
    *)
        echo "\tUnknown architecture: $ARCH"
        exit 1
        ;;
esac

# get Android version
ANDROID_VERSION=$(getprop ro.build.version.release)
echo "Android version: $ANDROID_VERSION"