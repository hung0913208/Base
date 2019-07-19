#!/bin/bash
# - File: install.sh
# - Description: This bash script will be used to install a repo from scratch

PIPELINE="$(dirname "$0" )"
SCRIPT="$(basename "$0")"

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
	if [ $# -gt 1 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1 script ${SCRIPT}"
	fi
	exit -1
}
unameOut="$(uname -s)"
case "${unameOut}" in
	Linux*)     machine=Linux;;
	Darwin*)    machine=Mac;;
	CYGWIN*)    machine=Cygwin;;
	MINGW*)     machine=MinGw;;
	FreeBSD*)   machine=FreeBSD;;
	*)          machine="UNKNOWN:${unameOut}"
esac

ROOT="$(git rev-parse --show-toplevel)"
CURRENT="$(pwd)"

case "${unameOut}" in
	Linux*)     PARALLEL=$(($(grep -c ^processor /proc/cpuinfo)*2)) VERBOSE=1;;
	Darwin*)    PARALLEL=$(($(sysctl hw.ncpu | awk '{print $2}')*2)) VERBOSE=1;;
	CYGWIN*)    PARALLEL=1;;
	MINGW*)     PARALLEL=1;;
	FreeBSD*)   PARALLEL=$(($(sysctl hw.ncpu | awk '{print $2}')*2)) VERBOSE=1;;
	*)          PARALLEL=1;;
esac

# @TODO: check on storage if we have any artifact to reuse, we will use it to
# avoid rebuild again since rebuild take so much time

# @NOTE: cloning step
if ! git clone --recurse-submodules -j"$PARALLEL" "$1" &> /dev/null; then
	error "can't clone repo $1"
fi

# @NOTE: install step
cd "$CURRENT/$(basename "$1")" || error "cannot jump to $CURRENT/$(basename "$1")"

if [ ! -f "$ROOT/Tests/Pipeline/Packages/$(basename "$1").sh" ]; then
	error "not found "$ROOT/Tests/Pipeline/Packages/$(basename "$1").sh""
fi

bash "$ROOT/Tests/Pipeline/Packages/$(basename "$1").sh"
cd "$CURRENT" || error "cannot jump to $CURRENT"

# @TODO: store artifact to reuse again

# @NOTE: done
info "Congratulation, you have done install $(basename "$1"), please verify again"
