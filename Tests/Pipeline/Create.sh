#!/bin/bash
# - File: Create.sh
# - Description: we would like to assume that we may install on an default
# system with limited packages and we must have root permission or can access
# to root easily

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

if [ "$1" = "reproduce" ] || [ "$1" = "build" ]; then
	PACKAGES=$4
	BRANCH=$3
	REPO=$2
	JOB=$1

	if [ $1 = "reproduce" ]; then
		install_package screen
		install_package lftp
	fi
else
	PACKAGES=$3
	BRANCH=$2
	REPO=$1
	JOB="build"
fi

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

function start() {
	SAVE=$IFS
	IFS=$'\n'

	HOOKS=($(printenv | grep HOOK))
	IFS=$SAVE
	
	info "Start a new job $JID"
	if [[ ${#HOOKS[@]} -gt 0 ]]; then
		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" = 'HOOK_TOP' ]; then
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
				break
			fi
		done

		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" != 'HOOK_BOT' ] && [ "$NAME" != 'HOOK_TOP' ] && [[ ! "$NAME" =~ "NOTIFY"* ]] && [[ ! "$NAME" =~ "DEPART"* ]]; then
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
			fi
		done

		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" = 'HOOK_BOT' ]; then	
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
				break
			fi
		done
	fi
}

function notify() {
	SAVE=$IFS
	IFS=$'\n'

	HOOKS=($(printenv | grep NOTIFY))
	IFS=$SAVE

	info "Receive the job $JID"

	if [[ ${#HOOKS[@]} -gt 0 ]]; then
		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" = 'NOTIFY_TOP' ]; then
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
				break
			fi
		done

		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" != 'NOTIFY_BOT' ] && [ "$NAME" != 'NOTIFY_TOP' ] && [[ ! "$NAME" =~ "HOOK"* ]] && [[ ! "$NAME" =~ "DEPART"* ]]; then
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
			fi
		done

		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" = 'NOTIFY_BOT' ]; then	
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
				break
			fi
		done
	fi
}

function stop() {
	SAVE=$IFS
	IFS=$'\n'

	HOOKS=($(printenv | grep HOOK))
	IFS=$SAVE
	
	info "Stop the job $JID"
	if [[ ${#HOOKS[@]} -gt 0 ]]; then
		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" = 'DEPART_TOP' ]; then
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
				break
			fi
		done

		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" != 'DEPART_BOT' ] && [ "$NAME" != 'DEPART_TOP' ] && [[ ! "$NAME" =~ "NOTIFY"* ]] && [[ ! "$NAME" =~ "HOOK"* ]]; then
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
			fi
		done

		for ITEM in "${HOOKS[@]}"; do
			NAME=$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[0:a.find('=')]);")

			if [ "$NAME" = 'DEPART_BOT' ]; then	
				echo "$(echo "$ITEM" | python -c "import sys; a = sys.stdin.readlines()[0]; print(a[a.find('=') + 1:]);")" >> ./HOOK
				break
			fi
		done
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

start
if [ -f ./HOOK ]; then
	$SU chmod +x ./HOOK

	if [ $? = 0 ]; then
		source ./HOOK

		if [ $? != 0 ]; then
			info "script HOOK is corrupted please check again i will do with default configure"
			cat ./HOOK
		fi
	fi

	rm -fr ./HOOK
fi

notify
if [ -f ./HOOK ]; then
	$SU chmod +x ./HOOK

	if [ $? = 0 ]; then
		if ! bash ./HOOK; then	
			info "script HOOK is corrupted please check again i will do with default configure"
			cat ./HOOK
		fi
	fi

	rm -fr ./HOOK
fi

for CMD in $CMDS; do
	if [ ! $(which "$CMD") ]; then
		METHOD=0
	fi
done

if [[ ${#PACKAGES} -gt 0 ]]; then
	for PACKAGE in "${PACKAGES[@]}"; do
		install_package "$PACKAGE"
	done
fi

if [[ ${#JOB} -gt 0 ]]; then
	if [[ "$JOB" != "reproduce" ]] && [[ "$JOB" != "build" ]]; then
		exit 0
	fi
fi

# @NOTE: build a CI system with a qemu image
if [[ $METHOD -le 1 ]] && [ $(which qemu-img) ]; then
	CMDS=("bridge-utils" "iptables" "expect" "iproute2" "uml-utilities" "wput" "wget" "flex" "bison" "bc" "screen")
	ELFUTILS=("libelf-dev" "libelf-devel" "elfutils-libelf-devel")
	PASSED=1

	source $PIPELINE/Libraries/QEmu.sh

	# @NOTE: install specific packages to support qemu
	for CMD in "${CMDS[@]}"; do
		install_package $CMD

		if [ $? != 0 ]; then
			PASSED=0
		fi
	done

	for CMD in "${ELFUTILS[@]}"; do
		install_package $ELFUTILS

		if [ $? = 0 ]; then
			break;
		fi
	done

	BRIDGE=$(get_new_bridge "br")
	MASTER=$(get_internet_interface)

	if [ $PASSED != 1 ]; then
		warning "can't install requirement packages to support qemu"
	fi

	# @NOTE: create a new bridge, this will be used in the cases we want to
	# make a dificult network
	create_bridge "$BRIDGE"
	if [ $? != 0 ]; then
		warning "your environemt don't support creating a bridge"
		MODE="nat"
	else
		$SU ip link delete $BRIDGE type bridge
		MODE="bridge"
	fi

	if [ $(which depmod) ]; then
		DEPMOD="depmod"
	else
		DEPMOD=$($SU find /sbin/ -type f -name "depmod")

		if [[ ${#DEPMOD} -eq 0 ]]; then
			warning "can't find depmod"
			DEPMOD=""
		fi
	fi

	# @NOTE: load module TUN/TAP
	if [ $(which modprobe) ]; then
		MODPROBE="modprobe"
	else
		MODPROBE=$($SU find /sbin/ -type f -name "modprobe")

		if [[ ${#MODPROBE} -eq 0 ]]; then
			warning "can't find modprobe"
			MODPROBE=""
		fi
	fi

	if [[ ${#MODPROBE} -gt 0 ]]; then
		$SU $MODPROBE tun

		if [ $? != 0 ]; then
			warning "can't load module TUN/TAP"
			MODE="isolate"
		fi
	else
		MODE="isolate"
	fi

	METHOD=4
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

	if [ -f ./HOOK ]; then
		"$PIPELINE/Environments/$CMD" $JOB $PIPELINE $MODE
	else
		"$PIPELINE/Environments/$CMD" $JOB $PIPELINE $MODE $REPO $BRANCH
	fi

	CODE=$?
else
	echo "[  ERROR  ]: Broken pipeline, not found $PIPELINE/Environemts/$CMD"
	CODE=-1
fi

stop
if [ -f ./HOOK ]; then
	$SU chmod +x ./HOOK

	if [ $? = 0 ]; then
		if ! bash ./HOOK; then
			info "script HOOK is corrupted please check again i will do with default configure"
			cat ./HOOK
		fi
	fi

	rm -fr ./HOOK
fi
exit $CODE
