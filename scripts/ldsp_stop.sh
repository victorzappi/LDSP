#!/bin/bash
#
# This script stops the LDSP program running on the phone

kill $(ps | grep LineageDSP | grep -Eo '[0-9]{2,5}' | grep -Eo '[0-9]{2,5}' -m 1)

# ps -> prints all running processes
# grep LineageDSP ->  prints all info of LineageDSP process as a line
# grep -Eo '[0-9]{2,5}' -> extracts all numerical entries [including pid] as different lines [pid is first one]
# grep -Eo '[0-9]{2,5}' -m 1 -> extracts numerical entries but only on first line, effectively returning pid

# we can only use basi grep on android! no awk, no sed, no perl-style grep

# extraction of numerical entries: https://askubuntu.com/a/184216
# apply grep on first line only: https://stackoverflow.com/a/69833537
# 'pipe' grep's output into kill: https://stackoverflow.com/a/3510850


