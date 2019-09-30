#!/bin/bash
# File: CPU.sh
# Description: This script is called as plugin for monitoring reproducing usage 

if [[ $# -gt 0 ]] && [ $1 = 'reset' ]; then
	rm -fr /tmp/ram.txt
#elif [[ $# -gt 1 ]] && [ $1 = 'report' ]; then
#	if [ -f /tmp/ram.txt ]; then
#		cat /tmp/ram.txt
#	fi
elif [[ $# -eq 0 ]]; then
	while [ 1 ]; do
		echo $(free -h | grep "Mem:" | awk '{ print $7 }') > /tmp/ram.txt
		sleep 10
	done
fi
