#!/bin/bash
# Searches recursively for word 'mixer' in all files from dir passed as argument
# it does not use find nor grep
# usage:
# sh ldsp_mixerPaths_recursive.sh /directory/to/initial/search

search_dir() {
    for file in "$1"/*; do
        if [ -d "$file" ]; then
            search_dir "$file"
        elif [ -f "$file" ]; then
            case "${file}" in
                *mixer*)
                    echo "Found candidate: $file"
                    ;;
            esac
        fi
    done
}

# Check if an argument was provided
if [ "$#" -eq 1 ]; then
	echo "Searching mixer paths files in $1"
    search_dir "$1"
else
    echo "Usage: $0 <initial-directory>"
    exit 1
fi

