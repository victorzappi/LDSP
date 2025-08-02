#!/system/bin/sh
# helper script designed to be called by ldsp_mixerPaths_opened.sh
# executable that receives all activities on files from inotifyd
# and prints opened files with "mixer_paths" in the names
# Arguments: EVENT FILE [SUBFILE]

# Only act on “r” (IN_OPEN) events *and* filenames containing “mixer_paths”
if [ "$1" = "r" ] && echo "$2" | grep -q "mixer_paths"; then
  printf '\tMixer paths file candidate: %s\n' "$2"
fi


