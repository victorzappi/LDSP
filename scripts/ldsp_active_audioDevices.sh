#!/bin/bash
#
# This script prints the names and ids of all active devices on card 0
# as well as the current hardware parameters

# returns the list of active devices, with multiple occurences
pcm=( $(grep -v closed /proc/asound/card0/*/sub0/status | grep -Eo 'pcm[0-9]{1,2}.') )

# get len of list
len="${#pcm[@]}"

# in case there are not active devices
if [ "$len" -eq "0" ];
then
	echo 'No active devices on card 0'
    exit 0
fi


echo 'Active pcm devices on card 0:'
echo

# prepare to loop throgh list
len=$(($len-1))
# we will ignore multiple occurences by checking that name is different than previous
prev=''

ii=0
while [[ $ii -le $len ]]
do
   curr=${pcm[$ii]}
   if [ "$curr" != "$prev" ]; then
   		# grab last character, 'p' for playback and 'c' for capture
   		dir=${curr: -1}
   		if [ "$dir" == 'p' ]; then
   			dir='Playback device:'
   		else
   			dir='Capture device:'
   		fi
   		# grab the id of the device, with multiple occurrences each preceded by path
		id=( $(grep -E 'id:' /proc/asound/card0/*/info | grep $curr) )
		echo $dir $curr '('${id[1]}')' # the second element is the first occurence of the id
	fi 
	prev=$curr
    ii=$(($ii+1))

done


echo
echo 'Device parameters:'
echo
grep -v closed /proc/asound/card0/*/sub0/hw_params





# grep commands adapted from here: https://github.com/meefik/linuxdeploy/issues/223#issuecomment-339116666

