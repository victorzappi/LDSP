#!/bin/bash
#
# This script prints the numbers and ids of all active devices (capture and playback) 
# on card 0, their current hardware parameters and, if a plaback device is active,
# also the ldsp command line arguments to replicate this configuration
# usage:
# sh ldsp_active_audioDevices.sh




# command line arguments
# shared across playback and capture
rate=0			# sample rate
per=0		 	# period size
buf=0 			# period count (size*count=buffer size)
format=''
# different for playback and capture
num=(-1 -1)
chn=(-1 -1)

# command line argument parameters
# shared across playback and capture
ratearg='-r'	# sample rate
perarg='-p' 	# period size
bufarg='-b' 	# period count (size*count=buffer size)
formatarg='-f'	# format
# different for playback and capture
idarg=("-s" "-S") #VIC string array must have double quotes
numarg=("-d" "-D") #VIC string array must have double quotes
chnarg=("-n" "-N") #VIC string array must have double quotes

# param positions in hw_params file
ratepos=9
perpos=12
bufpos=14
chnpos=7
formatpos=3


extract_id()
{
	id_full=$1 # this contains id: MUTIPLE_STRINGS (*)
	last="${#id_full[@]}"	
	id='' # we will concat all strings here
	jj=1 # skil 'id:'
	# cycle all strings and concat
	while [[ $jj -lt $last ]]
	do
		if [[ "${id_full[jj]}" != "(*)" ]]; then
			id="$id ${id_full[jj]}"
		fi
		jj=$(($jj+1))
	done
	echo $id
}



print_params()
{
	file=$1
	dir=$2
	# path where all params of device are stored
	path="/proc/asound/card0/$file/sub0/hw_params"
	# get them all, as a list
	params=( $(cat $path) ) 


	# these params are overwritte if both capture and playback device are active
	# but we expect them to be the same for both devices!
	rate=${params[$ratepos]}
	per=${params[$perpos]}
	buf=${params[$bufpos]}
	buf=$(($buf/$per)) # this is actually the buffer count
	format=${params[$formatpos]}
	
	chn[$dir]=${params[$chnpos]}
	# device number has been retrieved and displayed already

	echo '   ''rate:' $rate '('${ratearg}')'
	echo '   ''period:' $per '('${perarg}')'
	echo '   ''buffer count:' $buf '('${bufarg}')'
	echo '   ''format:' $format '('${formatarg}')'
	echo '   ''channel count:' ${chn[$dir]} '('${chnarg[$dir]}')'

	#DEBUG prtints all params
#	for curr in "${params[@]}"
#	do
#		echo $curr
#	done
}






#-----------------------------------------------------------------------------------------

# returns the list of active devices (folder names), with multiple occurences
pcm=( $(grep -v closed /proc/asound/card0/*/sub0/status | grep -Eo 'pcm[0-9]{1,2}.') )

# get len of list
len="${#pcm[@]}"

# in case there are not active devices
if [ "$len" -eq "0" ];
then
	echo 'No active devices on card 0'
    exit 0
fi


echo 'Active pcm devices on card 0'

# prepare to loop through list of active devices (folder names)
len=$(($len-1))
# we will ignore multiple occurences by checking that current name is different than previous
prev=''

capfound=0
playfound=0


#ii=0
#while [[ $ii -le $len ]]
for curr in "${pcm[@]}" 
do
   #curr=${pcm[$ii]}
   if [ "$curr" != "$prev" ]; then
   		# grab last character as stream direction, 'p' for playback and 'c' for capture
   		dir=${curr: -1}
   		if [ "$dir" == 'p' ]; then
   			dir=0
   			playfound=1
   			echo
   			echo 'Playback device'
   		else
   		   	dir=1
   		   	capfound=1
   		   	echo
   			echo 'Capture device'
   		fi
   		# grab the number of the device (from 0 to 999!)
		tmp=( $(echo $curr | grep -Eo '[0-9]{1,3}') )
   		num[$dir]=$tmp
   		# grab the id of the device
   		# first get list with multiple occurrences, each preceded by path
		#id=( $(grep -E 'id:' /proc/asound/card0/*/info | grep $curr) )
		#id=${id[1]} # the second element is the first occurence of the id
		id_full=( $(grep id /proc/asound/card0/${curr}/info) )
		id=$(extract_id $id_full)
		echo '   ''number:' ${num[$dir]} '('${numarg[$dir]}')'
		echo '   ''id:' $id '('${idarg[$dir]}')'
		echo '   ''folder name:' $curr 
		# function called only once per type of device (cap or playback)
		print_params $curr $dir
	fi 
	prev=$curr
    #ii=$(($ii+1))
done

capargs=''

if [ "$capfound" == 0 ]; then
	echo
	echo '   ''output only (-O)'
	capargs='-O' # if capture is not found, we use 'output only' argument
else
	# otherwise, we concat number and channel count for capture device (capture specific args)
	capargs="${numarg[1]} ${num[1]} ${chnarg[1]} ${chn[1]}"
fi

# if a playback device is active, we print the command line arguments to replicate config
if [ "$playfound" == 1 ]; then
	# playback specific args, concat number and channel count for playback device
	playargs="${numarg[0]} ${num[0]} ${chnarg[0]} ${chn[0]}"
	echo
	echo 'Equivalent command line arguments:'
	# first soecific args, than shared ones
	echo $playargs $capargs ${ratearg} ${rate} ${perarg} ${per} ${bufarg} ${buf} ${formatarg} ${format} 
fi



# grep commands adapted from here: https://github.com/meefik/linuxdeploy/issues/223#issuecomment-339116666

