#!/system/bin/sh
#
# This checks if any MIDI devices are detected and, in case, prints the relative ports
# usage:
# sh ldsp_midiDevices.sh



ii=0
for ff in /dev/snd/*; do

	ff=${ff##/dev/snd/} # remove path and get file name
	
	# look for files that have 'midi' in the name, like midiC1D0
	if [[ $ff == *midi* ]]; then
		# store file name
		midifiles[$ii]=$ff
		#echo ${midifiles[$ii]}
		
		ii=$(($ii+1))
	fi
done


if [[ $ii == 0 ]]; then
	echo 'No MIDI devices detected!'
	echo 'Either no MIDI devices are connected or the phone does not support MIDI /:'
	exit 0
fi


len=$ii
ii=0
echo 'MIDI devices detected:'

while [[ $ii -lt $len ]]
do
	echo '   ''port:' ${midifiles[$ii]}

	ii=$(($ii+1))
done






