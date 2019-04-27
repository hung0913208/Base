#!/bin/bash
./bootstrap

case "$(uname -s)" in
	Linux*)     make -j$(($(grep -c ^processor /proc/cpuinfo)*2));;
  Darwin*)    PARALLEL=$(($(sysctl hw.ncpu | awk '{print $2}')*2));;
	CYGWIN*)    make;;
	MINGW*)     make;;
	FreeBSD*)   make -j$(($(sysctl hw.ncpu | awk '{print $2}')*2));;
	*)          make;;
esac

sudo make install
