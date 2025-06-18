#!/system/bin/sh
#
# This script stops the remote debugger ('lldb-server') running on the phone
# usage:
# sh ldsp_debugserver_stop.sh

# for older phones
PID=$(ps | grep lldb-server | grep -Eo '[0-9]{2,5}' | grep -Eo '[0-9]{2,5}' -m 1)
if [[ -n "$PID" ]]; then
  kill "$PID"
fi

# for android >= 9 we need to add -A
# unfortunately this extra argument breaks ps on older phones, so we run both versions
PID=$(ps -A | grep lldb-server | grep -Eo '[0-9]{2,5}' | grep -Eo '[0-9]{2,5}' -m 1)
if [[ -n "$PID" ]]; then
  kill "$PID"
fi 
# ps -> prints all running processes
# grep lldb-server -> prints all info of lldb-server process as a line
# grep -Eo '[0-9]{2,5}' -> extracts all numerical entries [including pid] as different lines [pid is first one]
# grep -Eo '[0-9]{2,5}' -m 1 -> extracts numerical entries but only on first line, effectively returning pid

# we can only use basic grep on android! no awk, no sed, no perl-style grep

# extraction of numerical entries: https://askubuntu.com/a/184216
# apply grep on first line only: https://stackoverflow.com/a/69833537
# 'pipe' grep's output into kill: https://stackoverflow.com/a/3510850


