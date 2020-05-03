#!/bin/bash

if which cgdb &> /dev/null; then
	GDB=$(which cgdb)
elif which gdb &> /dev/null; then
	GDB=$(which gdb)
else
	exit -1
fi

for PID in $(ps -aux | grep "valgrind\|gdbserver" | grep -v "grep\|debug.sh" | awk '{ print $2 }'); do
	kill -9 $PID
done

if [[ $# == 3 ]] && [[ ! -f ./ngrok ]]; then
	export NGROK_AUTHTOKEN=$3

	rm -fr ngrok.zip
	rm -fr ngrok

	unameOut="$(uname -s)"
	case "${unameOut}" in
		Linux*)     curl -o ngrok.zip https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-linux-amd64.zip;;
		Darwin*)    wget -o ngrok.zip https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-darwin-amd64.zip;;
		MINGW*)     wget -o ngrok.zip https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-windows-amd64.zip;;
		FreeBSD*)   wget -o ngrok.zip https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-freebsd-amd64.zip;;
		*)          exit 1
		esac
	unzip ngrok.zip
fi

if [ -z $NGROK_AUTHTOKEN ]; then
	# @NOTE: perform debug without using ngrok

	if [ $1 == "gdbserver" ]; then
		# @NOTE: perform debug with gdbserver
		gdbserver localhost:1234 ./$2 &>/dev/null
	elif [ $1 == "valgrind" ]; then
		# @NOTE: perform debug with valgrind and gdb/cgdb, please don't use it on CI

		(valgrind --tool=memcheck --leak-check=full  --vgdb=full --vgdb-error=0 ./$2 &>/dev/null) & \
			(sleep 5 && $GDB -ex 'target remote | vgdb')

		pkill --signal 9 -f /usr/bin/valgrind.bin
	else
		echo "don\'t support $1"
	fi
elif [ ! -f $2 ]; then
	echo "Your build is fail at the compiling step"
else
	#  @NOTE: kill all ngrok if it still exists and register authtoken
	pkill --signal 9 -f ./ngrok
	./ngrok authtoken $NGROK_AUTHTOKEN

	if [ $1 == "gdbserver" ]; then
	# @NOTE: perform debug with gdbserver and ngrok as a tunning tool

	((gdbserver localhost:5678 $2 &>/dev/null) && pkill --signal 9 -f ./ngrok) & \
	(./ngrok tcp 5678 &>/dev/null)

	elif [ $1 == "valgrind" ]; then
	# @NOTE: perform debug with valgrind and ngrok as a tunning tool

	((valgrind --tool=memcheck --leak-check=full --vgdb=full --vgdb-error=0 $2) && \
		pkill --signal 9 -f ./ngrok) & \
		(sleep 5 && (vgdb --port=1234 &>/dev/null)) & (./ngrok tcp 1234 &>/dev/null)
	else
		echo "don\'t support $1"
	fi

	pkill --signal 9 -f /usr/bin/valgrind.bin
	pkill --signal 9 -f gdbserver
fi

if [ -f ./ngrok ]; then
	rm -fr ngrok.zip
	rm -fr ngrok
fi
