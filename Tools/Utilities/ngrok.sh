#!/bin/bash

PASSWORD=$1
TOKEN=$2
PORT=$3

if [ ! -e ./ngrok-stable-libnux-amd64.zip ]; then
	wget -q -c -nc https://bin.equinox.io/c/4VmDzA7iaHb/ngrok-stable-linux-amd64.zip
fi

if [ ! -e ./ngrok ]; then
	unzip -qq -n ngrok-stable-linux-amd64.zip
fi

echo root:$PASSWORD | chpasswd >& /dev/null
mkdir -p /var/run/sshd

echo "PermitRootLogin yes" >> /etc/ssh/sshd_config
echo "PasswordAuthentication yes" >> /etc/ssh/sshd_config
echo "LD_LIBRARY_PATH=/usr/lib64-nvidia" >> /root/.bashrc
echo "export LD_LIBRARY_PATH" >> /root/.bashrc

./ngrok authtoken $TOKEN >& /dev/null
./ngrok tcp 22 &
NGROK_PID=$!

sleep 10
curl -s http://localhost:4040/api/tunnels | python3 -c \
	"import sys, json; print(json.load(sys.stdin)['tunnels'][0]['public_url'])"

for IDX in {0..6}; do
	echo "this message is showed to prevent ci hanging"

	ps -p $NGROK_PID >& /dev/null
	if [ $? != 0 ]; then
		break
	else
		sleep 540
	fi
done
