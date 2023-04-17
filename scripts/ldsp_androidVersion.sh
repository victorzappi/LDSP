#!/bin/bash
#
# Prints the version of Android running on this device,
# if the property has been filled by the vendor or whomever built the ROM
# sh ldsp_androidVersion.sh


ANDROID_VERSION=$(getprop ro.build.version.release)
echo "Android version: $ANDROID_VERSION"


exit 0



