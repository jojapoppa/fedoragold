#!/bin/bash
PATH=/usr/bin:$PATH
DATE=$(date '+%d-%m-%Y %H:%M:%S')

# time must exceed duration of rescans and reset... (30 mins is good)
if ! (http --timeout 1800 127.0.0.1 | grep "Peers")
then
echo -e "checkhttp.sh: $DATE: re-Starting server"
#/root/reboot.sh
else
echo -e "checkhttp.sh: $DATE: FED Explorer working OK..."
fi
