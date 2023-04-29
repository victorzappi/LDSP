#!/bin/bash
#
# This script copies all resources pushed to SD card to main LDSP folder
# Useful on phones where file system cannot be romounted as read/write, e.g., when adb insecure fails
# sh ldsp_install_fromSD.sh

cp /sdcard/ldsp/* /data/ldsp

