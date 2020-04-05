#!/bin/bash
# - File: build.sh
# - Description: This bash script will be run right after prepare.sh and it will
# be used to build based on current branch you want to Tests

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

if [[ $# -gt 0 ]]; then
	MODE=$2
else
	MODE=0
fi

info "You have run on machine ${machine} script ${SCRIPT}"
info "Your current dir now is $(pwd)"
if [ $(which git) ]; then
	# @NOTE: jump to branch's test suite and perform build
	ROOT="$(git rev-parse --show-toplevel)"

	if [[ $# -gt 0 ]]; then
		PROJECT=$1

		if [ $PROJECT != "$(basename `git config  --get remote.origin.url`)" ]; then
			info "Use custom name $PROJECT"
		fi
	else
		PROJECT="$(basename `git config  --get remote.origin.url`)"
	fi

	if [[ $PROJECT == 'LibBase' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'LibBase.git' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Base' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Base.git' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'base' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'base.git' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'libbase' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'libbase.git' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Eevee' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Eevee.git' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Eden' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ $PROJECT == 'Eden.git' ]]; then
		if [ $REROUTE = 1 ]; then
			ROOT=$(pwd)
		fi

		BUILDER="${ROOT}/Tools/Builder/build"
	elif [[ -d "${ROOT}/Base" ]]; then
		BUILDER="${ROOT}/Base/Tools/Builder/build"
	elif [[ -d "${ROOT}/base" ]]; then
		BUILDER="${ROOT}/base/Tools/Builder/build"
	elif [[ -d "${ROOT}/libbase" ]]; then
		BUILDER="${ROOT}/libbase/Tools/Builder/build"
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
		- ${ROOT}/libbase
		- ${ROOT}/LibBase
		- ${ROOT}/Eevee
		- ${ROOT}/Eden
		"""
	fi

	CODE=0

	# @NOTE: build with bazel
	if which bazel &> /dev/null; then
		if [ -f $ROOT/WORKSPACE ]; then
			if ! bazel build ...; then
				CODE=-1
			elif ! bazel test ...; then
				CODE=-1
			fi
		fi
	fi

	if [[ ${CODE} -ne 0 ]]; then
		# @NOTE: do this action to prevent using Builder

		rm -fr $ROOT/{.workspace, WORKSPACE}

		# @NOTE: build and test everything with single command only

		if ! $BUILDER --root $ROOT --debug 1 --rebuild 0 --mode $MODE; then
			exit -1
		fi

		exit -1
	else
		# @NOTE: build and test everything with single command only

		if ! $BUILDER --root $ROOT --debug 1 --rebuild 0 --mode 2; then
			exit -1
		fi
	fi
else
	error "Please install git first"
fi

info "Congratulation, you have passed ${SCRIPT}"
