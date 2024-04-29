#!/bin/bash
#
# This script retrieves the phone's code name, android version and architecture
# to fill in the hw config file and use ldsp.sh correctly
# usage:
# sh ldsp_phoneDetails.sh



# get mode code name
MODEL_NAME=$(getprop ro.product.model)
echo "Phone code name (model): $MODEL_NAME"


# get android version
ANDROID_VERSION=$(getprop ro.build.version.release)
echo "Android version: $ANDROID_VERSION"



#-----------------------------------------------------------

# returns info about processor, as a list of words
arch=( $(cat /proc/cpuinfo | grep Processor) )



# get len of list
len="${#arch[@]}"

# if the list is empty, something went wrong
if [ "$len" -eq "0" ];
then
	echo 'Could not get processor info ):'
    exit 0
fi

# architecture is third element, after semicolon
if [ "${arch[2]}" == 'AArch64' ]; then
	echo 'target architecture: aarch64'
	echo 'supports neon floating point unit: true' # aarch64 always supports neon
	exit 0
elif [ "${arch[2]}" == 'ARMv7' ]; then
	echo 'target architecture: armv7a'
else
	echo 'Could not get architecture info ):'
	exit 0
fi


# if armv7, check for neon support
# neon can be listed in features
feat=( $(cat /proc/cpuinfo | grep Features) )

# get len of list
len="${#feat[@]}"

# prepare to loop through list
len=$(($len-1))

ii=0
# look for neon entry
while [[ $ii -le $len ]]
do
	curr=${feat[$ii]}
	# listed as either 'neon'
	if [ "$curr" != "neon" ]; then
   		echo 'supports neon floating point unit: true'
		exit 0
	# or 'asimd'
	elif [ "$curr" != "asimd" ]; then
   		echo 'supports neon floating point unit: true'
		exit 0
   	fi
    ii=$(($ii+1))
done

# if neither strings is found, no neon support
echo 'supports neon floating point unit: false'

