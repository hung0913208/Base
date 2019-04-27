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

	for IF_PATH in /sys/class/net/*; do
		echo $IF_PATH | grep "^$1" >& /dev/null

		if [ $? != 0 ]; then
			continue
		else
			ID=$(echo $(basename $IF_PATH) | sed 's/[^0-9]*//g')
		fi
	done

	echo "$1$(($ID + 1))"
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
