#!/bin/bash
# - File: build.sh
# - Description: This bash script will be run right after prepare.sh and it will
# be used to build based on current branch you want to Tests

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

info "You have run on machine ${machine} script ${SCRIPT}"
info "Your current dir now is $(pwd)"

if [ $(which git) ]; then
	# @NOTE: jump to branch's test suite and perform build
	ROOT="$(git rev-parse --show-toplevel)"
	PROJECT="$(basename `git config  --get remote.origin.url`)"

	if [[ $PROJECT == 'LibBase' ]]; then
		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'LibBase.git' ]]; then
		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Eevee' ]]; then
		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Eevee.git' ]]; then
		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ -d "${ROOT}/Base" ]]; then
		BUILDER="${ROOT}/Base/Tools/Builder/build"
	elif [[ -d "${ROOT}/base" ]]; then
		BUILDER="${ROOT}/base/Tools/Builder/build"
	elif [[ -d "${ROOT}/LibBase" ]]; then
		BUILDER="${ROOT}/LibBase/Tools/Builder/build"
	elif [[ -d "${ROOT}/Eden" ]]; then
		BUILDER="${ROOT}/Eden/Tools/Builder/build"
	elif [[ -d "${ROOT}/Eevee" ]]; then
		BUILDER="${ROOT}/Eevee/Tools/Builder/build"
	else
		error """
		Not found LibBase, it must be inside
		- ${ROOT}/Base
		- ${ROOT}/base
		- ${ROOT}/LibBase
		- ${ROOT}/Eevee
		- ${ROOT}/Eden
		"""
	fi

	# @NOTE: build and test everything with single command only
	$BUILDER --root $ROOT --debug 1 --rebuild 0 --mode $1
	if [ $? != 0 ]; then
		exit -1
	fi
else
	error "Please install git first"
fi

info "Congratulation, you have passed ${SCRIPT}"
