#!/bin/bash
#
# Searches for mixer paths file in the path passed via command line
# if not path is passed, searches /etc/ and /vendor/etc
# usage:
# sh ldsp_mixerPaths.sh /path/to/search


search_path()
{
	path=$1
	
	# Add a '*' to the end of the path
	path_="${path}*"
	
	ii=0
	for ff in $path_; do

		ff=${ff##$path} # remove path and get file name
		
		# look for xml files that have 'mixer' in the name
		if [[ $ff == *mixer*.xml ]]; then
			# store file name
			mixerfiles[$ii]=$ff
			
			ii=$(($ii+1))
		fi
	done
	
	echo $ii
	echo "${mixerfiles[@]}"
}


#-----------------------------------------------------------------------------------------

if [ -n "$1" ]; then
	userpath=$1
	# Check if the last character of the path is a '/'
	if [[ "${userpath: -1}" != "/" ]]; then
	  # If it's not, add a '/' to the end of the path
	  userpath="${userpath}/"
	fi
	
	echo "Searching mixer paths files in $userpath"
	output=$(search_path $userpath)

	# read the first element of the array
	read -r len <<< "$output"

	# read the remaining elements of the array
	remaining_elements=()
	while read -r element; do
	  remaining_elements+=("$element")
	done <<< "${output#*$len}"
	
	#VIC unfortunately, it turns out that we only get:
	# element 0 -> empty
	# element 1 -> list of all files, separated by spaces

	mixerfiles=${remaining_elements[1]} # we will split the files from here

	if [[ $len -gt 0 ]]; then

		echo "Mixer paths files found:"
		# use IFS to set the delimiter to a space
		IFS=" "

		# loop through the strings in mixerfiles
		for file in $mixerfiles; do
		  echo '   ' $userpath$file
		done
	else
		echo "No mixer paths files found"
	fi
	exit 0
fi





# deafault search 1
defaultpath1="/etc/"

echo "Searching mixer paths files in $defaultpath1"
output1=$(search_path $defaultpath1)

# read the first element of the array
read -r len1 <<< "$output1"

# read the remaining elements of the array
remaining_elements1=()
while read -r element; do
  remaining_elements1+=("$element")
done <<< "${output1#*$len1}"

mixerfiles1=${remaining_elements1[1]} 

if [[ $len1 -gt 0 ]]; then

	echo "Mixer paths files found:"
	# use IFS to set the delimiter to a space
	IFS=" "

	# loop through the strings in mixerfiles
	for file in $mixerfiles1; do
		  echo '   ' $defaultpath1$file
	done
else
	echo "No mixer paths files found"
fi





# deafault search 2
defaultpath2="/etc/vendor"

echo ""
echo "Searching mixer paths files in $defaultpath2"
output2=$(search_path $defaultpath2)

# read the first element of the array
read -r len2 <<< "$output2"

# read the remaining elements of the array
remaining_elements2=()
while read -r element; do
  remaining_elements2+=("$element")
done <<< "${output2#*$len2}"

mixerfiles2=${remaining_elements2[1]} 

if [[ $len2 -gt 0 ]]; then

	echo "Mixer paths files found:"
	# use IFS to set the delimiter to a space
	IFS=" "

	# loop through the strings in mixerfiles
	for file in $mixerfiles2; do
		  echo '   ' $defaultpath2$file
	done
else
	echo "No mixer paths files found"
fi




exit 0



