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
echo "	device: $BRAND $MODEL"

# determine architecture source: props or uname
ABI_PROP=$(getprop ro.product.cpu.abi)
if [ -n "$ABI_PROP" ]; then
    # use ABI prop to set ARCH
    case "$ABI_PROP" in
        arm64-v8a)
            ARCH="aarch64" ;;
        armeabi-v7a)
            ARCH="armv7a" ;;
        armeabi)
            ARCH="armv5te" ;;
        x86)
            ARCH="x86" ;;
        x86_64)
            ARCH="x86_64" ;;
        *)
            echo "Unknown abi '$ABI_PROP', cannot configure LDSP ):"
            exit 1 ;;
    esac
else
    # fallback to uname for ARCH
    if command -v uname >/dev/null 2>&1; then
        RAW_ARCH=$(uname -m)
        case "$RAW_ARCH" in
            aarch64)
                ARCH="aarch64" ;;
            armv7l|armv8l)
                ARCH="armv7a" ;;
            armv5te*)
                ARCH="armv5te" ;;
            i386|i686)
                ARCH="x86" ;;
            x86_64)
                ARCH="x86_64" ;;
            *)
                echo "Unknown architecture '$RAW_ARCH', incompatible with LDSP ):"
                exit 1 ;;
        esac
    else
        echo "Could not detect architecture, cannot configure LDSP ):"
        exit 1
    fi
fi

echo "	target architecture: $ARCH"


echo "Additional info:"

# determine NEON support based on ARCH
case "$ARCH" in
    aarch64|armv7a)
        NEON_STATUS="	NEON floating point unit present" ;;
    armv5te|x86|x86_64)
        NEON_STATUS="	NEON floating point unit not present" ;;
    # *)
    #     NEON_STATUS="	Unknown NEON support for $ARCH" ;;
esac

echo "$NEON_STATUS"

# print Android version last
ANDROID_VERSION=$(getprop ro.build.version.release)
echo "	Android version: $ANDROID_VERSION"
