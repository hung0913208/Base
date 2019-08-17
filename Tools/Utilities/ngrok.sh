#!/bin/bash

TOKEN=$2
PASS=$3
WAIT=$4
PORT=$5

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

if [ ! -f ./ngrok-stable-linux-amd64.zip ]; then
	timeout 30 wget https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-linux-amd64.zip

	if [ $? != 0 ]; then
		if [[ ${#USERNAME} -gt 0 ]] && [[ ${#PASSWORD} -gt 0 ]]; then
			timeout 30 wget ftp://${USERNAME}:${PASSWORD}@ftp.drivehq.com/Repsitory/ngrok-stable-linux-amd64.zip
		else
			exit -1
		fi
	fi

	if [[ $? -ne 0 ]]; then
		exit -1
	fi
fi

screen -ls "ngrok.pid" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print $1}' | while read s; do screen -XS $s quit; done
if [ ! -f ./ngrok ]; then
	unzip -qq -n ngrok-stable-linux-amd64.zip
fi

if [ "$1" = "ssh" ]; then
	# @NOTE: check root if we didn"t have root permission
	if [ $(whoami) != "root" ]; then
		if [ ! $(which sudo) ]; then
			error "Sudo is needed but you aren't on root and can't access to root"
		fi

		if sudo -S -p "" echo -n < /dev/null; then
			SU="sudo"
		else
			error "Sudo is not enabled"
		fi
	fi

	echo root:$PASS | $SU chpasswd
	$SU mkdir -p /var/run/sshd

	echo "PermitRootLogin yes" | $SU tee -a /etc/ssh/sshd_config >& /dev/null
	echo "PasswordAuthentication yes" | $SU tee -a /etc/ssh/sshd_config >& /dev/null
	echo "LD_LIBRARY_PATH=/usr/lib64-nvidia" | $SU tee -a /root/.bashrc >& /dev/null
	echo "export LD_LIBRARY_PATH" | $SU tee -a /root/.bashrc >& /dev/null


	if netstat -tunlp | grep sshd; then
		PORT=$(netstat -tunlp | grep sshd | awk '{ print $4; }' | awk -F":" '{ print $2; }') >& /dev/null
	else
		SSHD=$(which sshd) >& /dev/null

		if which sshd >& /dev/null; then
			if [ ${#PORT} = 0 ]; then
				read LOWER UPPER < /proc/sys/net/ipv4/ip_local_port_range

				for (( PORT = 1 ; PORT <= LOWER ; PORT++ )); do
					timeout 1 nc -l -p "$PORT"

					if [ $? = 124 ]; then
						break
					fi
				done
			fi

			$SSHD -E /tmp/sshd.log -p $PORT
		fi
	fi
elif [ "$1" = "netcat" ]; then
	PID="$(pgrep -d "," nc.traditional)"
	WAIT=$4

	if [[ ${#PID} -gt 0 ]]; then
		kill -9 $PID >& /dev/null
	fi

	if which nc.traditional; then
		(nohup $(which nc.traditional) -vv -o ./$PORT.log -lek -q -1 -i -1 -w -1 -c /bin/bash -r)&
	fi

	sleep 1
	PORT=$(netstat -tunlp | grep nc.traditional | awk '{ print $4; }' | awk -F":" '{ print $2; }') >& /dev/null
fi

./ngrok authtoken $TOKEN >& /dev/null
if [[ $WAIT -gt 0 ]]; then
	./ngrok tcp $PORT &
	PID=$!
else
	screen -S "ngrok.pid" -dm $(pwd)/ngrok tcp $PORT
fi

sleep 3

if netstat -tunlp | grep ngrok >& /dev/null; then
	curl -s http://localhost:4040/api/tunnels | python3 -c \
		"import sys, json; print(json.load(sys.stdin)['tunnels'][0]['public_url'])"
else
	exit -1
fi

if [[ $WAIT -gt 0 ]]; then
	for IDX in {0..40}; do
		echo "this message is showed to prevent ci hanging"

		kill -0 $PID >& /dev/null
		if [ $? != 0 ]; then
			break
		else
			sleep 60
		fi
	done

	kill -9 $PID

	if [ "$1" = "netcat" ]; then
		PID="$(pgrep -d "," nc.traditional)"
	
		if [[ ${#PID} -gt 0 ]]; then
			kill -9 $PID >& /dev/null
		fi
	fi
fi

