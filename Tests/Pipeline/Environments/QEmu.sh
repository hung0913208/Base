#!/bin/bash

MODE=$5
REPO=$3
METHOD=$1
BRANCH=$4
PIPELINE=$2

if [ -z "$KERNEL_URL" ]; then
	KERNEL_URL="git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux"
fi

if [ -z "$KERNEL_BRANCH" ]; then
	KERNEL_BRANCH="master"
fi

source $PIPELINE/Libraries/QEmu.sh
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"
INIT_DIR=$PIPELINE
ROOT="$(git rev-parse --show-toplevel)"

BBOX_URL="https://busybox.net/downloads/busybox-1.30.1.tar.bz2"
BBOX_FILENAME=$(basename $BBOX_URL)
BBOX_DIRNAME=$(basename $BBOX_FILENAME ".tar.bz2")

KERNEL_NAME=$(basename $KERNEL_URL)
KERNEL_DIRNAME=$(basename $KERNEL_NAME ",git")

git branch | egrep 'Pipeline/QEmu$' >& /dev/null
if [ $? != 0 ]; then
	CURRENT=$(pwd)

	cd $PIPELINE || error "can't cd to $PIPELINE"
	git branch -a | egrep 'remotes/origin/Pipeline/QEmu$' >& /dev/null

	if [ $? = 0 ]; then
		# @NOTE: by default we will fetch the maste branch of this environment
		# to use the best candidate of the branch

		git fetch >& /dev/null
		git checkout -b "Pipeline/QEmu" "origin/Pipeline/QEmu"
		if [ $? = 0 ]; then
			"$0" $METHOD $PIPELINE $REPO $BRANCH $MODE
		fi

		exit $?
	fi

	cd $CURRENT || error "can't cd to $CURRENT"
fi

function compile_busybox() {
	cd "$INIT_DIR"
	curl "$BBOX_URL" -o "$BBOX_FILENAME" 2>/dev/null
	tar xf "$BBOX_FILENAME"

	cd "$INIT_DIR/$BBOX_DIRNAME/"
	make defconfig >& /dev/null
	sed -i "/CONFIG_STATIC/c\CONFIG_STATIC=y" ./.config

	make -j8 >& /dev/null
	make install >& /dev/null
	cd "$INIT_DIR"
}

function generate_isolate_initscript(){
	if [ "$METHOD" == "reproduce" ]; then
		cat > "$INIT_DIR/$BBOX_DIRNAME/initramfs/init" << EOF
#!/bin/busybox sh
mount -t proc none /proc
mount -t sysfs none /sys

echo "The virtual machine's memory usage:"
free -m

# @NOTE: increase maximum fd per process
ulimit -n 65536

# @NOTE: config loopback interface
ifconfig lo 127.0.0.1
route add 127.0.0.1
EOF
	else
		cat > "$INIT_DIR/$BBOX_DIRNAME/initramfs/init" << EOF
#!/bin/busybox sh
mount -t proc none /proc
mount -t sysfs none /sys

echo "The virtual machine's memory usage:"
free -m

# @NOTE: increase maximum fd per process
ulimit -n 65536

# @NOTE: config loopback interface
ifconfig lo 127.0.0.1
route add 127.0.0.1
EOF
	fi
}

function generate_network_initscript(){
	if [ "$METHOD" == "reproduce" ]; then
		cat > "$INIT_DIR/$BBOX_DIRNAME/initramfs/init" << EOF
#!/bin/busybox sh
mount -t proc none /proc
mount -t sysfs none /sys

echo "The virtual machine's memory usage:"
free -m

# @NOTE: increase maximum fd per process
ulimit -n 65536

# @NOTE: config nameserver
echo "nameserver 8.8.8.8" > /etc/resolv.conf

# @NOTE: config loopback interface
ifconfig lo 127.0.0.1
route add 127.0.0.1

# @NOTE: config eth0 interface to connect to outside
ifconfig eth0 up
ifconfig eth0 192.168.100.2
route add 192.168.100.1

ping -c 5 192.168.100.1 >& /dev/null
if [ $? != 0 ]; then
	echo "can't connect to the gateway"
fi

ping -c 5 8.8.8.8 >& /dev/null
if [ $? != 0 ]; then
	echo "can't connect to internet"
fi

ping -c 5 google.com >& /dev/null
if [ $? != 0 ]; then
	echo "can't access to DNS server"
fi
EOF
	else
		cat > "$INIT_DIR/$BBOX_DIRNAME/initramfs/init" << EOF
#!/bin/busybox sh
mount -t proc none /proc
mount -t sysfs none /sys

echo "The virtual machine's memory usage:"
free -m

# @NOTE: increase maximum fd per process
ulimit -n 65536

# @NOTE: config nameserver
echo "nameserver 8.8.8.8" > /etc/resolv.conf

# @NOTE: config loopback interface
ifconfig lo 127.0.0.1
route add 127.0.0.1

# @NOTE: config eth0 interface to connect to outside
ifconfig eth0 up
ifconfig eth0 192.168.0.2
route add default gw 192.168.100.1

ping -c 5 192.168.0.1 >& /dev/null
if [ $? != 0 ]; then
	echo "can't connect to the gateway"
fi

ping -c 5 8.8.8.8 >& /dev/null
if [ $? != 0 ]; then
	echo "can't connect to internet"
fi

ping -c 5 google.com >& /dev/null
if [ $? != 0 ]; then
	echo "can't access to DNS server"
fi
EOF
	fi
}

function generate_initrd() {
	local COUNT=0

	cd "$INIT_DIR/$BBOX_DIRNAME/"
	cp -rf _install/ initramfs/ >& /dev/null
	cp -fR examples/bootfloppy/etc initramfs >& /dev/null

	rm -f initramfs/linuxrc
	mkdir -p initramfs/{dev,proc,sys,tests} >& /dev/null
	$SU cp -a /dev/{null,console,tty,tty1,tty2,tty3,tty4} initramfs/dev/ >& /dev/null

	# @NOTE: generate our initscript which is used to call our testing system
	if [ $MODE != "isolate" ]; then
		generate_network_initscript
	else
		generate_isolate_initscript
	fi
	chmod a+x initramfs/init

	# @NOTE: copy only execuators that don't support dynamic link
	for FILE in $(find $1 -executable); do
		if [ -d "$FILE" ]; then
			continue
		fi

		if [ -x "$FILE" ]; then
			ldd $FILE >& /dev/null

			if [ $? != 0 ]; then
				COUNT=$(($COUNT + 1))

				cp $FILE initramfs/tests
				echo "chroot . ./tests/$(basename $FILE)" >> initramfs/init
			fi
		fi
	done

	if [ $COUNT != 0  ]; then
		# @NOTE: add footer of the initscript
		echo "sleep 80" >> initramfs/init
		echo "poweroff -f" >> initramfs/init

		# @NOTE: okey, everything is done from host machine, from now we should
		# pack everything into a initramfs.cpio.gz. We will use it to deploy a
		# simple VM with qemu and the kernel we have done building
		exec 2>&-
		cd $INIT_DIR/$BBOX_DIRNAME/initramfs

		find . -print0 | cpio --null -ov --format=newc | gzip -9 > "${RAM_FILENAME}"
		cd $INIT_DIR
		exec 2>&1
	fi

	cd "${INIT_DIR}/${BBOX_DIRNAME}/initramfs/"
	cd "$INIT_DIR"
}

function compile_linux_kernel() {
	if [[ ${#FTP} -gt 0 ]]; then
		wget "$FTP" -o $KERNEL_NAME.tar.gz

		info "fetch kernel from $FTP"
		if [ $? = 0 ]; then
			tar xf "$KERNEL_NAME.tar.gz" -C "$INIT_DIR/$KERNEL_DIRNAME"

			if [ $? = 0 ]; then
				rm -fr "$KERNEL_NAME.tar.gz"
			fi
		fi
	else
		info "build a specific kernel"
	fi

	cd "$INIT_DIR"

	# @NOTE: by default, we should use linux as convention kernel to deploy
	# a minimal system
	if [ -d "$INIT_DIR/$KERNEL_DIRNAME" ]; then
		cd "$INIT_DIR/$KERNEL_DIRNAME"
		git pull origin $KERNEL_BRANCH
	else
		git clone --branch $KERNEL_BRANCH $KERNEL_URL
		cd "$INIT_DIR/$KERNEL_DIRNAME"
	fi

	# @NOTE: by default we will use default config of linux kernel but we
	# will use the specify if it's configured
	if [ ! -z $KERNEL_CONFIG ]; then
		curl $KERNEL_CONFIG -o ./.config
	elif [ -f $ROOT/.config ]; then
		cp $ROOT/.config ./.config
	else
		make defconfig
	fi

	unameOut="$(uname -s)"
	case "${unameOut}" in
		Linux*)     make -j$(($(grep -c ^processor /proc/cpuinfo)*16)) VERBOSE=1;;
		Darwin*)    make -j$(($(sysctl hw.ncpu | awk '{print $2}')*16)) VERBOSE=1;;
		CYGWIN*)    make -j16;;
		MINGW*)     make -j16;;
		FreeBSD*)   make -j$(($(sysctl hw.ncpu | awk '{print $2}')*16)) VERBOSE=1;;
		*)          make -j16;;
	esac

	if [[ ! -e ${KER_FILENAME} ]]; then
		error "can't create ${KER_FILENAME}"
	elif [[ ${#FTP} -gt 0 ]]; then
		tar -czf $ROOT/$KERNEL_NAME.tar.gz -C "$INIT_DIR/$KERNEL_DIRNAME" .

		if [ $? != 0 ]; then
			error "can't compress $KERNEL_NAME.tar.gz"
		fi

		wput -p -B  $ROOT/$KERNEL_NAME.tar.gz "$FTP"
		if [ $? != 0 ]; then
			warning "wput fail to upload to $FTP"
		fi

		rm -fr $ROOT/$KERNEL_NAME.tar.gz
	fi
	cd "$INIT_DIR"
}

function detect_libbase() {
	PROJECT=$1

	if [[ -d "${PROJECT}/Eden" ]]; then
		echo "${PROJECT}/Eden"
	elif [[ -d "${PROJECT}/Base" ]]; then
		echo "${PROJECT}/Base"
	elif [[ -d "${PROJECT}/base" ]]; then
		echo "${PROJECT}/base"
	elif [[ -d "${PROJECT}/LibBase" ]]; then
		echo "${PROJECT}/LibBase"
	elif [[ -d "${PROJECT}/Eevee" ]]; then
		echo "${PROJECT}/Eevee"
	else
		echo "$PROJECT"
	fi
}

if [ "$MODE" != "isolate" ]; then
	ETH="$(get_internet_interface)"
	TAP="$(get_new_bridge "tap")"

	create_tuntap "$TAP"

	if [ $? != 0 ]; then
		error "can\'t create a new tap interface"
	else
		# @NOTE: set ip address of this TAP interface
		$SU ip address add  192.168.0.1/24 dev $TAP

		# activate ip forwarding
		$SU sysctl net.ipv4.ip_forward=1
		$SU sysctl net.ipv6.conf.default.forwarding=1
		$SU sysctl net.ipv6.conf.all.forwarding=1

		# @NOTE: Create forwarding rules, where
		# tap0 - virtual interface
		# eth0 - net connected interface
		$SU iptables -A FORWARD -i $TAP -o $ETH -j ACCEPT
		$SU iptables -A FORWARD -i $ETH -o $TAP -m state --state ESTABLISHED,RELATED -j ACCEPT
		$SU iptables -t nat -A POSTROUTING -o $ETH -j MASQUERADE
	fi

	NETWORK="-net nic,model=e1000,vlan=0 -net tap,ifname=$TAP,vlan=0,script=no"
fi

mkdir -p "$ROOT/content"
cd "$ROOT/content"

if [ "$METHOD" == "reproduce" ]; then
	$PIPELINE/Reproduce.sh $REPO
	exit $?
elif [[ $# -gt 3 ]]; then
	BRANCH=$4
	info "import '$REPO $BRANCH' >> ./repo.list"

	echo "$REPO $BRANCH" >> ./repo.list
else
	# @NOTE: fetch the list project we would like to build from remote server
	if [ ! -f './repo.list' ]; then
		curl -k --insecure $REPO -o './repo.list' >> /dev/null
		if [ $? != 0 ]; then
			error "Can't fetch list of project from $REPO"
		fi
	fi
fi

cat './repo.list' | while read DEFINE; do
	SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
	REPO=${SPLITED[0]}
	BRANCH=${SPLITED[1]}
	PROJECT=$(basename $REPO)

	git clone --single-branch --branch $BRANCH $REPO $PROJECT
	if [ $? != 0 ]; then
		FAIL+=$PROJECT
	else
		ROOT=$(pwd)
		WORKSPACE="$ROOT/$PROJECT"

		# @NOTE: everything was okey from now, run CI steps
		cd "$WORKSPACE" || error "can't cd to $WORKSPACE"
		git submodule update --init --recursive

		BASE="$(detect_libbase $WORKSPACE)"
		mkdir -p "$WORKSPACE/build"

		# @NOTE: if we found ./Tests/Pipeline/prepare.sh, it can prove
		# that we are using Eevee as the tool to deploy this Repo
		if [ -e "$BASE/Tests/Pipeline/Prepare.sh" ]; then
			KER_FILENAME="$INIT_DIR/$KERNEL_DIRNAME/arch/x86_64/boot/bzImage"
			RAM_FILENAME="$INIT_DIR/initramfs.cpio.gz"

			$BASE/Tests/Pipeline/Prepare.sh

			if [ -f $WORKSPACE/.environment ]; then
				source $WORKSPACE/.environment
			fi

			if [ $? != 0 ]; then
				warning "Fail repo $REPO/$BRANCH"
			else
				$WORKSPACE/Tests/Pipeline/Build.sh 1

				if [ $? != 0 ]; then
					warning "Fail repo $REPO/$BRANCH"
				else
					# @NOTE: build a new initrd

					if [ ! -f $RAM_FILENAME ]; then
						compile_busybox
					fi

					generate_initrd "$WORKSPACE/build"
				fi
			fi
		else
			warning "repo $REPO don't support usual CI method"
		fi

		cd "$ROOT" || error "can't cd to $ROOT"
		rm -fr "$WORKSPACE"
	fi

	if [ -f "$RAM_FILENAME" ]; then
		if [ ! -f $KER_FILENAME ]; then
			compile_linux_kernel
		fi

		info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"
		qemu-system-x86_64 -s -kernel "${KER_FILENAME}" 	\
				-initrd "${RAM_FILENAME}"		\
				-nographic 				\
				-smp $(nproc) -m 1G			\
				$NETWORK				\
				-append "console=ttyS0 loglevel=8"

		# @TODO: start slave VMs at the same time with master VM using builder
		# to create a cluster for testing with different kind of network

		rm -fr "$RAM_FILENAME"
	else
		warning "i can't start QEmu because i don't see any approviated test suites"
	fi
done

rm -fr "$INIT_DIR/$BBOX_DIRNAME"
rm -fr "$INIT_DIR/$KERNEL_DIRNAME"

