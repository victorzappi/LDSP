#!/system/bin/sh
# Searches recursively for XML files whose name contains 'mixer' in dir passed as argument
# usage:
# sh ldsp_mixerPaths_recursive.sh /directory/to/initial/search

search_dir() {
    for file in "$1"/*; do
        if [ -d "$file" ]; then
            search_dir "$file"
        elif [ -f "$file" ]; then
            case "$file" in
                *mixer*.xml)
                    echo "Found mixer paths file: $file"
                    ;;
            esac
        fi
    done
}

# ——— argument check ———
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <initial-directory>"
    exit 1
fi

# ——— normalize ———
ROOT="${1%/}"       # strip any trailing slash; "/" → ""
if [ -z "$ROOT" ]; then
    DISPLAY="/"     # for user feedback only
else
    DISPLAY="$ROOT"
fi

echo "Searching for mixer XML files in $DISPLAY"
search_dir "$ROOT"