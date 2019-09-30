#!/bin/bash
# File: CPU.sh
# Description: This script is called as plugin for monitoring reproducing usage 

if [[ $# -gt 0 ]] && [ $1 = 'reset' ]; then
	rm -fr /tmp/cpu.txt
#elif [[ $# -gt 1 ]] && [ $1 = 'report' ]; then
#	if [ -f /tmp/cpu.txt ]; then
#		cat /tmp/cpu.txt
#	fi
elif [[ $# -eq 0 ]]; then
	while [ 1 ]; do
		echo $(grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage "%"}') >> /tmp/cpu.txt
		sleep 10
	done
fi
