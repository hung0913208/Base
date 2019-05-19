#!/bin/bash

set -e

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
	VERBOSE=OFF
fi

if [ $# == 0 ]; then
	if [ -f ./CMakeLists.txt ]; then
		cmake ./

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

if [ -f ../../CMakeLists.txt ]; then
	if [ $# -gt 1 ]; then
		cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=$VERBOSE -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} -DCMAKE_CXX_FLAGS="'$*'" ../..
	else
		cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=$VERBOSE -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} ../..
	fi
elif [ -f ../CMakeLists.txt ]; then
	if [ $# -gt 1 ]; then
		cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=$VERBOSE -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} -DCMAKE_CXX_FLAGS="'$*'" ../.
	else
		cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=$VERBOSE -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} ../.
	fi
else
	CMAKELIST=$2
	shift
	if [ $# -gt 0 ]; then
		cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=$VERBOSE -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} -DCMAKE_CXX_FLAGS="'$*'" ${CMAKELIST}
	else
		cmake -DCMAKE_VERBOSE_MAKEFILE:BOOL=$VERBOSE -DCMAKE_BUILD_TYPE=$BUILD_TYPE ${CONFIGURE} ${CMAKELIST}
	fi
fi

if [ $? -ne 0 ]; then
	code=-1
fi

unameOut="$(uname -s)"
case "${unameOut}" in
	Linux*)     make -j $(($(grep -c ^processor /proc/cpuinfo)*2)) VERBOSE=1;;
	Darwin*)    make -j $(($(sysctl hw.ncpu | awk '{print $2}')*2)) VERBOSE=1;;
	CYGWIN*)    make VERBOSE=1;;
	MINGW*)     make VERBOSE=1;;
	FreeBSD*)   make -j $(($(sysctl hw.ncpu | awk '{print $2}')*2)) VERBOSE=1;;
	*)          make VERBOSE=1;;
esac

if [ $? -ne 0 ]; then
	code=-1
fi

cd ..
exit $code


