#!/system/bin/sh
#
# looks in passed dir for mixer paths XML files that are opened by the audio HAL
# and lists them as official mixer paths candidates
# usage:
# sh ldsp_mixerPaths_opened.sh /directory/to/search

# Pick up user-passed dir or default, then remove trailing slash if any
if [ -n "$1" ]; then
  mixer_paths_dir="${1%/}"
else
  mixer_paths_dir="/vendor/etc"
fi

# Ensure helper script is executable
chmod +x /data/ldsp/scripts/ldsp_helper_printMixerPaths.sh

# Start inotifyd in the background, watching all XMLs under the dir
inotifyd /data/ldsp/scripts/ldsp_helper_printMixerPaths.sh "$mixer_paths_dir"/*.xml &
INOTIFY_PID=$!

echo "Checking mixer paths files in use in $mixer_paths_dir"

# Stop the audio HAL
stop audioserver
# Restart the audio HAL (triggers a fresh open of the XML mixer_paths file)
start audioserver

# Wait 5 seconds for the HAL to parse the XML
sleep 5

# Kill the inotifyd watcher
kill "$INOTIFY_PID" 2>/dev/null

