#!/bin/bash
# - File: qemu.sh
# - Description: this bash script will provide some functions to support
# configure network, build custom hda and cdrom images, deploy to a qemu
# virtual machine and interact with this machine

source $PIPELINE/Libraries/Logcat.sh

# @NOTE: check root if we didn"t have root permission
SU=""

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

function check_interface_status() {
	DNS_ID=("a" "b" "c" "d" "e" "f" "g" "h" "i" "j" "k" "l" "m")

	for ID in $DNS_ID; do
		ping -c 5 -I $1 "$ID.root-servers.net" >& /dev/null

		if [ $? = 0 ]; then
			return 0
		fi
	done

	return 1
}

function show_all_network_interface() {
	for IF_PATH in /sys/class/net/*; do
		INTERFACE="$(basename $IF_PATH)"

		# @NOTE: i found that brdctl always create this directory to
		# indicate this is a bridge interface
		if [ -d $IF_PATH/bridge ]; then
			continue
		else
			echo "$INTEFACE"
		fi
	done
}

function get_new_bridge() {
	ID=0

	while [ 1 ]; do
		if [ ! -e /sys/class/net/"$1""$ID" ]; then
			echo "$1""$ID"
			break
		else
			ID=$(($ID+1))
		fi
	done
}

function get_latest_bridge() {
	MAX=0
	ID=-1

	for IF_PATH in /sys/class/net/*; do
		echo $(basename $IF_PATH) | grep "^$1" >& /dev/null

		if [ $? != 0 ]; then
			continue
		else
			ID=$(echo $(basename $IF_PATH) | sed 's/[^0-9]*//g')

			if [[ $MAX -lt $ID ]]; then
				MAX=$ID
			fi
		fi
	done

	if [[ $ID -gt -1 ]]; then
		echo "$1$(($MAX))"
	else
		echo ""
	fi
}

function get_internet_interface() {
	# @NOTE: by default i will choose the first candidate as the internet
	# interface and i believe almost CI system will do that as an unspoken
	# rule

	for IF_PATH in /sys/class/net/*; do
		INTERFACE="$(basename $IF_PATH)"

		# @NOTE: i found that brdctl always create this directory to
		# indicate this is a bridge interface
		if [ -d $IF_PATH/bridge ]; then
			continue
		elif [ ! $(check_interface_status "$INTERFACE") ]; then
			echo "$INTERFACE"
			break
		fi
	done
}

function get_gateway() {
	if [ "$1" = 'default' ]; then
		ip route show | grep "$1" | awk '{ print $3 }'
	fi
}

function get_state_interface() {
	ip addr show $1 | grep state | awk '{ print $9 }'
}

function get_ip_interface() {
	ip addr show $1 | grep inet | awk '{ print $2 }' | awk '{ split($0,a,"/"); print a[1]; }'
}

function get_netmask_interface() {
	ip addr show $1 | grep inet | awk '{ print $2 }' | awk '{ split($0,a,"/"); print a[2]; }'
}

function get_range_interface() {
	python -c """
ip = '$1'
mask = $2

id = ip.split('.')
splited = bin(int(id[int(mask/8)]))[2:]

if len(splited) < 8:
	for i in range(len(splited), 8):
		splited = '0{}'.format(splited)

if mask % 8 > 0:
	splited = [c for c in splited]

	for i in range(0, 8 - mask % 8):
		splited[7 - i] = '1'
	else:
		splited = str(int(''.join(splited), 2))
else:
	splited = 255

result = ''

for i in range(4):
	if int(mask/8) == i:
		num = splited
	elif 8*i < mask:
		num = id[i]
	else:
		num = 255

	if len(result) == 0:
		result = str(num)
	else:
		result += '.{}'.format(str(num))
else:
	print('{} {}'.format(ip, result))
"""
}

function create_tuntap() {
	# @NOTE: add a new tuntap
	$SU tunctl -u $USER -t $1 >& /dev/null

	if [ $? != 0 ]; then
		return 1
	fi

	$SU ip link set dev $1 up
	if [ $? != 0 ]; then
		return 1
	fi

	return 0
}

function create_bridge() {
	# @NOTE: add a new bridge
	$SU brctl addbr $1 >& /dev/null

	if [ $? != 0 ]; then
		return 1
	else
		return 0
	fi
}

function make_bridge_slaving() {
	$SU ip link set dev $2 master $1

	if [ $? != 0 ]; then
		return 1
	else
		return 0
	fi
}

function snift() {
	if [ $(get_state_interface $1) = 'DOWN' ]; then
		return 1
	fi

	if which tcpdump >& /dev/null; then
		info "turn on snifting on $1"
		mkdir -p /tmp/dump

		if [ $ETH = $1 ]; then
			($SU tcpdump -vvi $1 -x >/tmp/dump/$1.tcap) & PID=$!
		else
			($SU tcpdump -nnvvi $1 -x >/tmp/dump/$1.tcap) & PID=$!
		fi

		if $SU ps -p $PID >& /dev/null; then
			echo "$PID" >> /tmp/tcpdump.pid
		fi
	else
		return 1
	fi
}

function start_dhcpd() {
	IP=$(get_ip_interface $1)
	MASK=$(get_netmask_interface $1)
	RANGE=($(get_range_interface $IP $MASK))

	if !which dnsmasq >& /dev/null; then
		return 1
	fi

	screen -S "dhcpd.pid" -dm 		\
		$SU dnsmasq --interface=$1 --bind-interfaces --dhcp-range=${RANGE[0]},${RANGE[1]}
}

function stop_dhcpd() {
	screen -ls "dhcpd.pid" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print \$1}' | while read s; do screen -XS \$s quit; done
}
