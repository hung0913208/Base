#!/bin/bash

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
    *)          machine="UNKNOWN:${unameOut}"
esac

BRANCH=$(git rev-parse --abbrev-ref HEAD)

if [[ $CC =~ '*clang*' ]] && [[ $CXX =~ '*clang++*' ]] && [ $machine != "Mac" ]; then
    error "we didn't support install gtest in this environment"
else
    if [ $(whoami) != "root" ]; then
        SU="sudo"
    fi

    cmake -DBUILD_SHARED_LIBS=ON .
    make && $SU make install

    if [ $machine == 'Linux' ]; then
        $SU ldconfig -v | grep gtest
    fi
fi
