#!/bin/bash
# - File: Create.sh
# - Description: we would like to assume that we may install on an default
# system with limited packages and we must have root permission or can access
# to root easily

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"
REPO=$1

probe_osx_machine() {
	# @NOTE: check if we are running inside a container

	if [ -f /.dockerenv ]; then
		info "You are running the test inside a container"
	else
		VM_PATTERNS=("VirtualBox" "Oracle" "VMware" "Parallels")
		IS_VIRTUAL_MACHINE=0
		VENDOR=$(ioreg -l | grep -e Manufacturer -e 'Vendor Name')

		for PATTERN in $VM_PATTERNS; do
			if [ "$VENDOR" = "$PATTERN" ]; then
				IS_VIRTUAL_MACHINE=1
			fi
		done

		if [ $IS_VIRTUAL_MACHINE != 0 ]; then
			info "You are running on virual machine $(ioreg -l | grep -e Manufacturer)"
		else
			info "You are running on physical machine $(ioreg -l | grep -e Manufacturer)"
		fi
	fi
}

probe_linux_machine(){
	# @NOTE: check if we are running inside a container

	if [ -f /.dockerenv ]; then
		info "You are running the test inside a container"
	else
		install_package "dmidecode"
		install_package "facter"

		# @NOTE: show information of this machine
		if [ $(which docker) ]; then
			info "The information of this machine:"
			$SU dmidecode -t system | grep 'Manufacturer\|Product'
		fi

		# @NOTE: check if we are running inside a virtual machine
		$SU facter 2> /dev/null | grep virtual >& /dev/null

		if [ $? != 0 ]; then
			info "You are runing inside a physical machine"
		else
			info "You are runing inside a virtual machine"
		fi
	fi
}

# @NOTE: now we will support these methods:
# - 0: unknown, we will force to close since we can't do anything now
# - 1: traditional way
# - 2: virtualbox way
# - 3: docker way
# - 4: qemu way

# @NOTE: check if we can build a CI system with the traditional way
CMDS=("git" "curl")
METHOD=1

for CMD in $CMDS; do
	if [ ! $(which "$CMD") ]; then
		METHOD=0
	fi
done

# @NOTE: build a CI system with a qemu image
if [[ $METHOD -le 1 ]] && [ $(which qemu-img) ]; then
	CMDS=("bridge-utils" "iptables" "expect" "iproute2")
	PASSED=1

	source $PIPELINE/Libraries/QEmu.sh

	# @NOTE: install specific packages to support qemu
	for CMD in $CMDS; do
		if [ $(install_package $CMD) ]; then
			PASSED=0
		fi
	done

	BRIDGE=$(get_new_bridge "br")
	MASTER=$(get_internet_interface)

	if [ $PASSED != 1 ]; then
		warning "can't install requirement packages to support qemu"
	fi

	# @NOTE: create a new bridge, this will be used in the cases we want to
	# make a dificult network
	if [ $PASSED != 0 ]; then
		create_bridge "$BRIDGE"

		if [ $? != 0 ]; then
			warning "your environemt don't support creating a bridge"
			PASSED=0
		fi
	fi

	# @NOTE: load module TUN/TAP
	if [ $PASSED != 0 ]; then
		$SU modprobe tun

		if [ $? != 0 ]; then
			warning "can't load module TUN/TAP"
			PASSED=0
		fi
	fi

	if [ $PASSED != 0 ]; then
		info """  welcome to the world of qemu system, since we have reonfigure your network, it may impact the whole
		network of your CI system. Thue, it will turn wild a litte bit. Sorry for any inconvenient"""

		METHOD=1 # @TODO: after we finish everything, we should switch to 3
	fi
fi

# @NOTE: build a CI system with a docker image
if [[ $METHOD -le 0 ]] && [ $(which docker) ]; then
	METHOD=3
fi


# @NOTE: build a CI system with a virtualbox image
if [[ $METHOD -le 0 ]] && [ $(which VBoxManage) ]; then
	METHOD=2
fi

# @NOTE: check again if the system is supported or not
case "${METHOD}" in
	"0")     error "Do nothing since we can't find the way to implement your system";;
	"1")     CMD='Traditional.sh';;
	"2")     CMD='Virtualbox.sh';;
	"3")     CMD='Docker.sh';;
	"4")     CMD='QEmu.sh';;
	*)       ;;
esac

if [ -e "$PIPELINE/Environments/$CMD" ]; then
	info "after checking i select script  $CMD to create a new enviroment"

	# @NOTE: show result of probing test to help to explain why i choose
	# the env script above
	case "$(uname -s)" in
		Linux*)	 	probe_linux_machine;;
		Darwin*)	probe_osx_machine;;
		CYGWIN*)	;;
		MINGW*)	 	;;
		FreeBSD*)   	;;
		*)		;;
	esac

	"$PIPELINE/Environments/$CMD" $PIPELINE $REPO
else
	error "Broken pipeline, not found $PIPELINE/Environemts/$CMD"
fi
