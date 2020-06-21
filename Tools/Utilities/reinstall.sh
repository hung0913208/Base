#!/bin/bash

CURRENT=$(pwd)

if [ -d "$1" ] && [[ $# -gt 1 ]]; then
	cd "$1"
	shift
fi

if [ -d ./$1 ]; then
	echo "Found old version build --> remove it now"

	if [ -d ./.git ]; then
		git clean -d -f -x
	elif [ ! -f ./CMakeLists.txt ]; then
		rm -fr ./$1/*
	fi
fi

if [ "$1" = "Coverage" ]; then
	unameOut="$(uname -s)"
	case "${unameOut}" in
		Linux*)     machine=Linux;;
		Darwin*)    machine=Mac;;
		CYGWIN*)    machine=Cygwin;;
		MINGW*)     machine=MinGw;;
		FreeBSD*)   machine=FreeBSD;;
		*)          machine="UNKNOWN:${unameOut}"
	esac
fi

if [ -z "$VERBOSE" ]; then
	VERBOSE=0
fi

if [ $# == 0 ]; then
	if [ -f $CURRENT/CMakeLists.txt ]; then
		cmake $CURRENT

		if [ $? != 0 ]; then
			exit -1
		fi

		make VERBOSE=1
		exit $?
	fi

	exit -1
fi

mkdir -p ./$1
cd ./$1
BUILD_TYPE=$1
shift

if [ -f $CURRENT/../CMakeLists.txt ]; then
	if [ $# -gt 1 ]; then
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} -DCMAKE_CXX_FLAGS="'$*'" $CURRENT/..
	else
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} $CURRENT/..
	fi
elif [ -f $CURRENT/CMakeLists.txt ]; then
	if [ $# -gt 1 ]; then
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} -DCMAKE_CXX_FLAGS="'$*'" $CURRENT/.
	else
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} $CURRENT/.
	fi
else
	CMAKELIST=$2
	shift
	if [ $# -gt 0 ]; then
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} -DCMAKE_CXX_FLAGS="'$*'" ${CMAKELIST}
	else
		cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} ${CMAKELIST}
	fi
fi

if [ $? -ne 0 ]; then
	code=-1
fi

if [[ $VERBOSE -eq 0 ]]; then
	unameOut="$(uname -s)"

	case "${unameOut}" in
		Linux*)     make -j $(($(grep -c ^processor /proc/cpuinfo)*2));;
		Darwin*)    make -j $(($(sysctl hw.ncpu | awk '{print $2}')*2));;
		CYGWIN*)    make;;
		MINGW*)     make;;
		FreeBSD*)   make -j $(($(sysctl hw.ncpu | awk '{print $2}')*2));;
		*)          make;;
	esac
else
	unameOut="$(uname -s)"

	case "${unameOut}" in
		Linux*)     make -j $(($(grep -c ^processor /proc/cpuinfo)*2)) VERBOSE=1;;
		Darwin*)    make -j $(($(sysctl hw.ncpu | awk '{print $2}')*2)) VERBOSE=1;;
		CYGWIN*)    make VERBOSE=1;;
		MINGW*)     make VERBOSE=1;;
		FreeBSD*)   make -j $(($(sysctl hw.ncpu | awk '{print $2}')*2)) VERBOSE=1;;
		*)          make VERBOSE=1;;
	esac
fi

if [ $? -ne 0 ]; then
	code=-1
fi

cd "$CURRENT"
exit $code


