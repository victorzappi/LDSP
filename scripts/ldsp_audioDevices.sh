#!/bin/bash
#
# This script prints the numbrs and ids of all active (capture and playback) on card 0
# usage:
# sh ldsp_audioDevices.sh


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


#-----------------------------------------------------------------------------------------


ip=0
ic=0
for ff in /proc/asound/card0/*; do

	ff=${ff##/proc/asound/card0/} # remove path and get folder name

	# get info playback devices
	
	# filter out folders that don't end with num and p --> must match pcmNp	
	if [[ $ff == *[0-9]p ]]; then
		# folder name
		p_folder[$ip]=$ff
		#echo ${p_folder[$ip]}
		
		# id
		id_full=( $(grep id /proc/asound/card0/${ff}/info) )
		p_id[$ip]=$(extract_id $id_full)
		#echo ${p_id[$ip]}
		
		# number (from 0 to 999!)
		num=( $(echo $ff | grep -Eo '[0-9]{1,3}') )
		p_num[$ip]=$num
		#echo ${p_num[$ip]}
		
		ip=$(($ip+1))
	fi
	
	
	# get info capture devices
		
	# filter out folders that don't end with num and c --> must match pcmNp	
	if [[ $ff == *[0-9]c ]]; then
		# folder name
		c_folder[$ic]=$ff
		#echo ${c_folder[$ic]}
		
		# id
		id_full=( $(grep id /proc/asound/card0/${ff}/info) )
		c_id[$ic]=$(extract_id $id_full)
		#echo ${c_id[$ic]}
		
		# number (from 0 to 999!)
		num=( $(echo $ff | grep -Eo '[0-9]{1,3}') )
		c_num[$ic]=$num
		#echo ${c_num[$ic]}
		
		ic=$(($ic+1))
	fi
done





len=$ip
ip=0
echo 'Playback devices:'

while [[ $ip -lt $len ]]
do
	echo '   ''number:' ${p_num[$ip]}
	echo '   ''id:' ${p_id[$ip]}
	echo '   ''folder name:' ${p_folder[$ip]}
	echo
	ip=$(($ip+1))
done

len=$ic
ic=0
echo
echo 'Capture devices:'

while [[ $ic -lt $len ]]
do
	echo '   ''number:' ${c_num[$ic]}
	echo '   ''id:' ${c_id[$ic]}
	echo '   ''folder name:' ${c_folder[$ic]}
	echo
	ic=$(($ic+1))
done


# iterating path: https://stackoverflow.com/a/21245538
# pattern matching: https://tldp.org/LDP/abs/html/string-manipulation.html
# function with return value: https://stackoverflow.com/a/17336953
# get return value from call with arguments: https://stackoverflow.com/a/6241286


