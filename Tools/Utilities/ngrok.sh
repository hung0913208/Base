#!/bin/bash

SERVICE=$1
WAIT=0
shift

# @NOTE: print log error and exit immediatedly
error(){
	if [ $# -eq 2 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1 in ${SCRIPT}"
	fi
	exit -1
}

login_pktriot() {
	if [ ! -f /tmp/pktriot.cookies ]; then
		LOGIN_TEMPFILE=$(mktemp /tmp/pktriot_login.XXXXXX)
	
		cat > $LOGIN_TEMPFILE << EOF
curl -sS --request POST -c /tmp/pktriot.cookies 'https://packetriot.com/login' -H 'content-type: application/x-www-form-urlencoded' --data 'email=$1&password=$2&google='
exit \$?
EOF

		bash $LOGIN_TEMPFILE
		CODE=$?

		rm -fr $LOGIN_TEMPFILE
		return $CODE
	fi

	return 0
}

get_pktriot_tunnels() {
	curl -sS --request GET --cookie /tmp/pktriot.cookies https://packetriot.com/tunnels | grep '<a class="black" href="/tunnel' | awk '{ split($0,a,"href=\"/tunnel/"); split(a[2],a,"\">"); print a[1]; }'
}

delete_pktriot_tunnel() {
	curl -sS --request GET --cookie /tmp/pktriot.cookies https://packetriot.com/tunnel/delete?id=$1 &> /dev/null
}

if [[ ${#WAIT} -eq 0 ]]; then
	WAIT=0
fi

screen -ls "${SERVICE}.pid" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print $1}' | while read s; do screen -XS $s quit; done

if [ $SERVICE = 'ngrok' ]; then
	while [ $# -gt 0 ]; do
		case $1 in
			--token)	NGROK_TOKEN="$2"; shift;;
			--wait)		WAIT=1; shift;;
			(--) 		shift; break;;
			(-*) 		error "unrecognized option $1";;
			(*)		METHOD="$1";;
		esac
		shift
	done

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

	if [ ! -f ./ngrok ]; then
		unzip -qq -n ngrok-stable-linux-amd64.zip
	fi
elif [ $SERVICE = 'pktriot' ]; then
	PKTRIOT_REGION="1"

	while [ $# -gt 0 ]; do
		case $1 in
			--email)	PKTRIOT_EMAIL="$2"; shift;;
			--password)	PKTRIOT_PASSWORD="$2"; shift;;
			--region)	PKTRIOT_REGION="$2"; shift;;
			(--) 		shift; break;;
			(-*) 		error "unrecognized option $1";;
			(*)		METHOD="$1";;
		esac
		shift
	done

	if [[ ${#PKTRIOT_EMAIL} -eq 0 ]] || [[ ${#PKTRIOT_PASSWORD} -eq 0 ]]; then
		exit -1
	fi

	if [ ! -f ./pktriot-0.9.10.amd64.tar.gz ]; then
		if ! timeout 30 wget https://pktriot-dl-bucket.sfo2.digitaloceanspaces.com/releases/linux/pktriot-0.9.10.amd64.tar.gz; then
			exit -1
		fi
	fi

	if [ ! -f ./pktriot ]; then
		if ! tar xf ./pktriot-0.9.10.amd64.tar.gz -C ./; then
			exit -1
		else
			cp ./pktriot-0.9.10/pktriot ./
		fi
	fi
else
	error "no support"
fi

if [ "$METHOD" = "ssh" ]; then
	PORT=""

	while [ $# -gt 0 ]; do
		case $1 in
			--port)		PORT="$2"; shift;;
			--password)	PASS="$2"; shift;;
			(--) 		shift; break;;
			(-*) 		error "unrecognized option $1";;
			(*) 		break;;
		esac
		shift
	done

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


	if netstat -tunlp | grep sshd &> /dev/null; then
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
elif [ "$METHOD" = "netcat" ]; then
	PID="$(pgrep -d "," nc.traditional)"
	WAIT=$4

	while [ $# -gt 0 ]; do
		case $1 in
			--port)		PORT="$2"; shift;;
			(--) 		shift; break;;
			(-*) 		error "unrecognized option $1";;
			(*) 		break;;
		esac
		shift
	done

	if [[ ${#PID} -gt 0 ]]; then
		kill -9 $PID >& /dev/null
	fi

	if which nc.traditional; then
		(nohup $(which nc.traditional) -vv -o ./${PORT}.log -lek -q -1 -i -1 -w -1 -c /bin/bash -r)&
	fi

	sleep 1
	PORT=$(netstat -tunlp | grep nc.traditional | awk '{ print $4; }' | awk -F":" '{ print $2; }') >& /dev/null
fi

if [ $SERVICE = 'ngrok' ]; then
	./ngrok authtoken $NGROK_TOKEN >& /dev/null

	if [[ $WAIT -gt 0 ]]; then
		./ngrok tcp $PORT &
		PID=$!
	else
		screen -S "ngrok.pid" -dm $(pwd)/ngrok tcp $PORT
	fi
elif [ $SERVICE = 'pktriot' ]; then
	rm -fr $HOME/.pktriot/config.json

	for I in {1..2}; do
		if expect -c """
set timeout 600

spawn $(pwd)/pktriot configure

expect \"Input selection\" { send \"3\n\" }
expect \"Email:\" { send \"$PKTRIOT_EMAIL\n\" }
expect \"Password:\" { send \"$PKTRIOT_PASSWORD\n\" }
expect \"Input selection\" { send \"$PKTRIOT_REGION\n\" }
expect {
	\"max tunnel quota hit\" { interact; exit -1 }
	\"Tunnel configuration:\" { interact; exit 0 }
}
""" &> /dev/null; then

			DONE=1
			break	
		fi

		if ! login_pktriot $PKTRIOT_EMAIL $PKTRIOT_PASSWORD; then
			break
		fi

		for TUNNEL in $(get_pktriot_tunnels); do
			delete_pktriot_tunnel $TUNNEL
		done

		DONE=0
	done

	if [[ $DONE -ne 1 ]]; then
		exit -1
	fi

	if ! ./pktriot tunnel tcp forward --destination 127.0.0.1 --dstport $PORT &> /dev/null; then
		exit -1
	fi

	if [[ $WAIT -gt 0 ]]; then
		./pktriot start &
		PID=$!
	else
		screen -S "pktriot.pid" -dm $(pwd)/pktriot start
	fi
fi

sleep 3

if [ $SERVICE = 'ngrok' ]; then
	if netstat -tunlp | grep ngrok >& /dev/null; then
		curl -s http://localhost:4040/api/tunnels | python3 -c \
			"import sys, json; print(json.load(sys.stdin)['tunnels'][0]['public_url'])"	
	else
		exit -1
	fi
elif [ $SERVICE = 'pktriot' ]; then
	RPORT=$(./pktriot route tcp ls | grep "| 22 " | awk '{ split($0,a,"|"); print a[2]; }' | sed -e 's/^[ \t]*//')
	DOMAIN=$(./pktriot info | grep "Hostname: " | awk '{ print $2 }')

	if [ $METHOD = 'ssh' ]; then
       		echo "ssh root@$DOMAIN -p $RPORT"
	elif [ $METHOD = 'netcat' ]; then
		echo "nc $DOMAIN $RPORT"
	fi
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

	if [ "$METHOD" = "netcat" ]; then
		PID="$(pgrep -d "," nc.traditional)"
	
		if [[ ${#PID} -gt 0 ]]; then
			kill -9 $PID >& /dev/null
		fi
	fi
fi

