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

mkdir -p $ROOT/build

# @NOTE: jump and perform building now
cd $ROOT/build || error "cannot jump to $ROOT/build"

# @NOTE: migrate Eevee to subproject Builder
cp -a $ROOT/Libraries $ROOT/Tools/Builder || error "cannot deliver $ROOT/Libraries to $ROOT/Tools/Builder"

# @NOTE: jump and perform building now
cd $ROOT/Tools/Builder || error "cannot jump to $ROOT/Tools/Builder"

# @NOTE: verify libraries that are used during install Builder 2
info "reinstall Builder 2 on Debug mode"

# @NOTE: disable stdout recently, we don't want to taint our console with unexpected messages
exec 2>&-
OUTPUT="$($ROOT/Tools/Utilities/reinstall.sh Debug 2>&1 | tee /dev/fd/2)"

if [[ $(grep "make\: \*\*\* \[all\] Error" <<< "$OUTPUT" ) ]]; then
	warning "fail build on Debug mode, please check console log bellow"
	echo "================================================================================="
	echo ""
	echo "$OUTPUT"
	echo "---------------------------------------------------------------------------------"
	exit -1
else
	$ROOT/Tools/Utilities/ctest.sh Debug
	if [ $? != 0 ]; then
		warning "fail test on Debug mode -> remove this candidate"
		rm -fr $ROOT/Tools/Builder/Debug
	fi
fi

info "reinstall Builder 2 on Release mode"
exec 2>&-
OUTPUT="$($ROOT/Tools/Utilities/reinstall.sh Release 2>&1 | tee /dev/fd/2)"

if [[ $(grep "make\: \*\*\* \[all\] Error" <<< "$OUTPUT" ) ]]; then
	warning "fail build on Release mode"
	echo "================================================================================="
	echo ""
	echo "$OUTPUT"
	echo "---------------------------------------------------------------------------------"
	exit -1
else
	$ROOT/Tools/Utilities/ctest.sh Release
	if [ $? != 0 ]; then
		warning "fail test on Release mode -> remove this candidate"
		rm -fr $ROOT/Tools/Builder/Debug
	fi
fi

if [ -x $ROOT/Tools/Builder/Release/build2 ]; then
	ln -s $ROOT/Tools/Builder/Release/build2 $ROOT/Tools/Builder/build2
elif [ -x $ROOT/Tools/Builder/Debug/build2 ]; then
	ln -s $ROOT/Tools/Builder/Debug/build2 $ROOT/Tools/Builder/build2
else
	warning "fail to install Builder 2"
	echo "================================================================================="
	echo ""
	echo "$OUTPUT"
	echo "---------------------------------------------------------------------------------"
	exit -1
fi

# @NOTE: back again
cd $CURRENT || error "cannot jump to $CURRENT"

