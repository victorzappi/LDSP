#!/system/bin/sh
#
# This script prints the numbers and ids of all active (capture and playback) on card 0
# usage:
# sh ldsp_audioDevices.sh

extract_id()
{
	id_full=$1 # this contains id: MULTIPLE_STRINGS (*)
	last="${#id_full[@]}"	
	id='' # we will concat all strings here
	jj=1 # skip 'id:'
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
		
		# id
		id_full=( $(grep id /proc/asound/card0/${ff}/info) )
		p_id[$ip]=$(extract_id $id_full)
		
		# number (from 0 to 999!)
		num=( $(echo $ff | grep -Eo '[0-9]{1,3}') )
		p_num[$ip]=$num
		
		ip=$(($ip+1))
	fi
	
	# get info capture devices
	
	# filter out folders that don't end with num and c --> must match pcmNc	
	if [[ $ff == *[0-9]c ]]; then
		# folder name
		c_folder[$ic]=$ff
		
		# id
		id_full=( $(grep id /proc/asound/card0/${ff}/info) )
		c_id[$ic]=$(extract_id $id_full)
		
		# number (from 0 to 999!)
		num=( $(echo $ff | grep -Eo '[0-9]{1,3}') )
		c_num[$ic]=$num
		
		ic=$(($ic+1))
	fi
done

# Sort playback devices using bubble sort
len=$ip
i=0
while [[ $i -lt $len ]]; do
	j=0
	while [[ $j -lt $(($len-$i-1)) ]]; do
		if [[ ${p_num[$j]} -gt ${p_num[$((j+1))]} ]]; then
			# Swap numbers
			temp=${p_num[$j]}
			p_num[$j]=${p_num[$((j+1))]}
			p_num[$((j+1))]=$temp
			
			# Swap ids
			temp="${p_id[$j]}"
			p_id[$j]="${p_id[$((j+1))]}"
			p_id[$((j+1))]="$temp"
			
			# Swap folder names
			temp=${p_folder[$j]}
			p_folder[$j]=${p_folder[$((j+1))]}
			p_folder[$((j+1))]=$temp
		fi
		j=$(($j+1))
	done
	i=$(($i+1))
done

# Sort capture devices using bubble sort
len=$ic
i=0
while [[ $i -lt $len ]]; do
	j=0
	while [[ $j -lt $(($len-$i-1)) ]]; do
		if [[ ${c_num[$j]} -gt ${c_num[$((j+1))]} ]]; then
			# Swap numbers
			temp=${c_num[$j]}
			c_num[$j]=${c_num[$((j+1))]}
			c_num[$((j+1))]=$temp
			
			# Swap ids
			temp="${c_id[$j]}"
			c_id[$j]="${c_id[$((j+1))]}"
			c_id[$((j+1))]="$temp"
			
			# Swap folder names
			temp=${c_folder[$j]}
			c_folder[$j]=${c_folder[$((j+1))]}
			c_folder[$((j+1))]=$temp
		fi
		j=$(($j+1))
	done
	i=$(($i+1))
done

# Print sorted playback devices
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

# Print sorted capture devices
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