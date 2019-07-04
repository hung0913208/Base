#!/bin/bash

PORT=$2
TOKEN=$3
PASSWORD=$4
WAIT=$5

# @NOTE: print log error and exit immediatedly
error(){
	if [ $# -eq 2 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1 in ${SCRIPT}"
	fi
	exit -1
}

if [[ ${#WAIT} -eq 0 ]]; then
	WAIT=0
fi

if [ ! -e ./ngrok-stable-libnux-amd64.zip ]; then
	wget -q -c -nc https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-linux-amd64.zip
fi

if [ ! -e ./ngrok ]; then
	unzip -qq -n ngrok-stable-linux-amd64.zip
fi

if [ "$1" = "ssh" ]; then
	# @NOTE: check root if we didn"t have root permission
	if [ $(whoami) != "root" ]; then
		if [ ! $(which sudo) ]; then
			error "Sudo is needed but you aren't on root and can't access to root"
		fi

		if sudo -S -p "" echo -n < /dev/null 2> /dev/null ; then
			SU="sudo"
		else
			error "Sudo is not enabled"
		fi
	fi

	$SU echo root:$PASSWORD | chpasswd >& /dev/null
	$SU mkdir -p /var/run/sshd

	$SU echo "PermitRootLogin yes" >> /etc/ssh/sshd_config
	$SU echo "PasswordAuthentication yes" >> /etc/ssh/sshd_config
	$SU echo "LD_LIBRARY_PATH=/usr/lib64-nvidia" >> /root/.bashrc
	$SU echo "export LD_LIBRARY_PATH" >> /root/.bashrc

	./ngrok authtoken $TOKEN >& /dev/null
	./ngrok tcp $PORT &
	PID=$!

	sleep 10
	curl -s http://localhost:4040/api/tunnels | python3 -c \
		"import sys, json; print(json.load(sys.stdin)['tunnels'][0]['public_url'])"

elif [ "$1" = "netcat" ]; then
	cat /tmp/netcat | /bin/sh -i 2>&1 | nc -l 127.0.0.1 $PORT > /tmp/netcat
	WAIT=0
fi

if [[ $WAIT -gt 0 ]]; then
	for IDX in {0..4}; do
		echo "this message is showed to prevent ci hanging"

		kill -0 $PID >& /dev/null
		if [ $? != 0 ]; then
			break
		else
			sleep 600
		fi
	done

	kill -9 $PID
fi
