#!/bin/bash

SCRIPT=$(basename $(pwd))

# @NOTE: print log info
info(){
	if [ $# -eq 2 ]; then
		echo "[   INFO  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[   INFO  ]: $1"
	fi
}

# @NOTE: print log warning
warning(){
	if [ $# -eq 2 ]; then
		echo "[ WARNING ]: $1 line ${SCRIPT}:$2"
	else
		echo "[ WARNING ]: $1"
	fi
}

# @NOTE: print log error and exit immediatedly
error(){
	if [ $# -eq 2 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1 in ${SCRIPT}"
	fi
	exit -1
}

# @NOTE: just to make sure that submodules will be fetched completely
git submodule update --init

ROOT="$(git rev-parse --show-toplevel)"
CURRENT=$(pwd)

# @NOTE: back again
cd $CURRENT || error "cannot jump to $CURRENT"

