#!/bin/bash
#
# This script converts a human-readable Android version name into an API level.
# usage:
# sh ldsp_convert_android_version.sh 7.0.0

version_full=$1

major=$(echo $version_full | cut -d . -f 1 )
minor=$(echo $version_full | cut -d . -f 2 )
patch=$(echo $version_full | cut -d . -f 3 )

if [[ $major == 1 ]]; then
	if [[ $minor == 0 ]]; then
		level=1
	elif [[ $minor == 1 ]]; then
		level=2
	elif [[ $minor == 5 ]]; then
		level=3
	elif [[ $minor == 6 ]]; then
		level=4
	fi
elif [[ $major == 2 ]]; then
	if [[ $minor == 0 ]]; then
		if [[ $patch == 1 ]]; then
			level=6
		else
			level=5
		fi
	elif [[ $minor == 1 ]]; then
		level=7
	elif [[ $minor == 2 ]]; then
		level=8
	elif [[ $minor == 3 ]]; then
		if [[ $patch == "" ]]; then
			level=9
		elif [[ $patch == 0 ]]; then
			level=9
		elif [[ $patch == 1 ]]; then
			level=9
		elif [[ $patch == 2 ]]; then
			level=9
		elif [[ $patch == 3 ]]; then
			level=10
		elif [[ $patch == 4 ]]; then
			level=10
		fi
	fi
elif [[ $major == 3 ]]; then
	if [[ $minor == 0 ]]; then
		level=11
	elif [[ $minor == 1 ]]; then
		level=12
	elif [[ $minor == 2 ]]; then
		level=13
	fi
elif [[ $major == 4 ]]; then
	if [[ $minor == 0 ]]; then
		if [[ $patch == "" ]]; then
			level=14
		elif [[ $patch == 0 ]]; then
			level=14
		elif [[ $patch == 1 ]]; then
			level=14
		elif [[ $patch == 2 ]]; then
			level=14
		elif [[ $patch == 3 ]]; then
			level=15
		elif [[ $patch == 4 ]]; then
			level=15
		fi
	elif [[ $minor == 1 ]]; then
		level=16
	elif [[ $minor == 2 ]]; then
		level=17
	elif [[ $minor == 3 ]]; then
		level=18
	elif [[ $minor == 4 ]]; then
		level=19
# API_LEVEL 20 corresponds to Android 4.4W ==  which isn't relevant to us
	fi
elif [[ $major == 5 ]]; then
	if [[ $minor == 0 ]]; then
		level=21
	elif [[ $minor == 1 ]]; then
		level=22
	fi
elif [[ $major == 6 ]]; then
	level=23
elif [[ $major == 7 ]]; then
	if [[ $minor == 0 ]]; then
		level=24
	elif [[ $minor == 1 ]]; then
		level=25
	fi
elif [[ $major == 8 ]]; then
	if [[ $minor == 0 ]]; then
		level=26
	elif [[ $minor == 1 ]]; then
		level=27
	fi
elif [[ $major == 9 ]]; then
	level=28
elif [[ $major == 10 ]]; then
	level=29
elif [[ $major == 11 ]]; then
	level=30
elif [[ $major == 12 ]]; then
# Android 12 uses both API_LEVEL 31 and 32
# Default to 31 ==  but allow user to say "12.1" to get 32
	if [[ $minor == 1 ]]; then
		level=32
	else
		level=31
	fi
elif [[ $major == 13 ]]; then
	level=33
fi

echo $level
