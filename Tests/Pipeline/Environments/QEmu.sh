#!/bin/bash
MODE=$3
REPO=$4
METHOD=$1
BRANCH=$5
PIPELINE=$2

if [ -z "$KERNEL_URL" ]; then
	KERNEL_URL="git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux"
fi

if [ -z "$KERNEL_BRANCH" ]; then
	KERNEL_BRANCH="master"
fi

if ! echo "$2" | grep ".reproduce.d" >& /dev/null; then
	if [ $1 = "inject" ] || [ -f $2 ] || [ ! -d $PIPELINE/Libraries ]; then
		if [ $(basename $0) = 'Environment.sh' ]; then
			PIPELINE="$(dirname $0)"
		else
			PIPELINE="$(dirname $0)/../"
		fi
	else
		PIPELINE=$2
	fi

	source $PIPELINE/Libraries/QEmu.sh
	source $PIPELINE/Libraries/Logcat.sh
	source $PIPELINE/Libraries/Package.sh
fi

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
	if ! echo "$2" | grep ".reproduce.d" >& /dev/null; then
		CURRENT=$(pwd)

		cd $PIPELINE || error "can't cd to $PIPELINE"
		git branch -a | egrep 'remotes/origin/Pipeline/QEmu$' >& /dev/null

		if [ $? = 0 ]; then
			# @NOTE: by default we will fetch the maste branch of this environment
			# to use the best candidate of the branch

			git fetch >& /dev/null
			git status | grep "Pipeline/QEmu"

			if [ $? != 0 ]; then
				git checkout -b "Pipeline/QEmu" "origin/Pipeline/QEmu"

				if [ $? = 0 ]; then
					cd $CURRENT || error "can't cd to $CURRENT"
					"$0" $METHOD $PIPELINE $MODE $REPO $BRANCH
				fi
				exit $?
			fi
		fi

		cd $CURRENT || error "can't cd to $CURRENT"
	fi
fi

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

function troubleshoot() {
	SCRIPT=$2

	if [ $1 = "start" ]; then
		if [ -d $SCRIPT ]; then
			if [ ! -f $SCRIPT/start ]; then
				return 0
			fi
		else
			return 0
		fi

		INTERFACES=($(show_all_network_interface))

		for IF_PATH in /sys/class/net/*; do
			snift $(basename $IF_PATH)
		done

		if [ -d $SCRIPT ]; then
			bash $SCRIPT/start
		else
			bash $SCRIPT start
		fi

	elif [ $1 = "end" ]; then
		if [ -d $SCRIPT ]; then
			if [ ! -f $SCRIPT/stop ]; then
				return 0
			fi
		else
			return 0
		fi

		if [ -f /tmp/tcpdump.pid ]; then
			cat /tmp/tcpdump.pid | while read PID; do
				$SU kill -15 $PID >& /dev/null
			done

			for FILE in /tmp/dump/*.tcap; do
				if [[ $(wc -c < $FILE) -gt 0 ]]; then
					info "tcpdump $(basename $FILE) from host view:"
					cat $FILE
					echo ""
				fi
			done

			rm -fr /tmp/tcpdump.pid
			rm -fr /tmp/dump
		fi

		if which iptables >& /dev/null; then
			info "iptables rule from host view:"
			$SU iptables -L
			$SU iptables -t nat --list-rules
			echo ""
		fi

		if which ifconfig >& /dev/null; then
			info "network configuration from host view:"
			$SU ifconfig
			echo ""
		else
			info "network configuration from host view:"
			$SU ip addr show
			echo ""
		fi

		if which route >& /dev/null; then
			info "routing table from host view:"
			$SU route -n
			echo ""
		else
			info "routing table from host view:"
			$SU ip route show
			echo ""
		fi

		if which arp >& /dev/null; then
			info "arp table from host view:"
			$SU arp
			echo ""
		else
			info "arp table from host view:"
			ip neigh show
			echo ""
		fi
		
		if [ -d $SCRIPT ]; then
			bash $SCRIPT/stop
		else
			bash $SCRIPT stop
		fi
	fi
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

if [ -f /network ]; then
	/network
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

if [ -f /network ]; then
	/network
fi
EOF
	fi
}

function generate_nat_initscript(){
	if [ "$METHOD" == "reproduce" ]; then
		cat > "$INIT_DIR/$BBOX_DIRNAME/initramfs/init" << EOF
#!/bin/busybox sh
mount -t proc none /proc
mount -t sysfs none /sys

echo "The virtual machine's memory usage:"
free -m

# @NOTE: increase maximum fd per process
ulimit -n 65536

if [ -f /network ]; then
	/network
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

if [ -f /network ]; then
	/network
fi
EOF
	fi
}

function generate_bridge_initscript(){
	if [ "$METHOD" == "reproduce" ]; then
		cat > "$INIT_DIR/$BBOX_DIRNAME/initramfs/init" << EOF
#!/bin/busybox sh
mount -t proc none /proc
mount -t sysfs none /sys

echo "The virtual machine's memory usage:"
free -m

# @NOTE: increase maximum fd per process
ulimit -n 65536

if [ -f /network ]; then
	/network
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

if [ -f /network ]; then
	/network
fi
EOF
	fi
}

function generate_netscript() {
	CLOSE=$3
	NET=$2
	IDX=$1

	if [ $MODE = "isolate" ]; then
		cat > $NET << EOF
#!/bin/sh

# @NOTE: config loopback interface
ifconfig lo 127.0.0.1
route add 127.0.0.1
EOF
	elif [ $MODE = "nat" ]; then
		TAP="$(get_new_bridge "tap")"
		create_tuntap "$TAP"

		if [ $? != 0 ]; then
			error "can\'t create a new tap interface"
		else
			# @NOTE: Create forwarding rules, where
			# tap0 - virtual interface
			# eth0 - net connected interface

			$SU iptables -A FORWARD -i $TAP -o $ETH -j ACCEPT
			$SU iptables -A FORWARD -i $ETH -o $TAP -m state --state ESTABLISHED,RELATED -j ACCEPT

			cat > $NET << EOF
EOF
		fi
	elif [ $MODE = "bridge" ]; then
		IDX=$((IDX+1))

		# @NOTE: set ip address of this TAP interface
		TAP="$(get_new_bridge "tap")"
		create_tuntap "$TAP"

		if [ $? != 0 ]; then
			error "can't create a new tap interface"
		else
			$SU ip link set dev $TAP master $BRD
			$SU ip link set $TAP up

			NETWORK="-device e1000,netdev=network$IDX -netdev tap,ifname=$TAP,id=network$IDX,script=no,downscript=no"

			cat > $CLOSE << EOF
#!/bin/sh

$SU ip addr flush dev $TAP
$SU ip link set $TAP down
$SU ovs-vsctl del-port $BRD $TAP
EOF

			cat > $NET << EOF
#!/bin/sh

# @NOTE: config nameserver
echo "nameserver 8.8.8.8" > /etc/resolv.conf

# @NOTE: config loopback interface
ifconfig lo 127.0.0.1

# @NOTE: config eth0 interface to connect to outside
ifconfig eth0 up

ifconfig eth0 192.168.1.$IDX
route add default gw 192.168.1.1 

echo "The VM's network configuration:"
ifconfig -a
echo ""

echo "The VM's routing table:"
route -n
echo ""

if ! ping -c 5 8.8.8.8 >& /dev/null; then
	echo "can't connect to internet"
fi

if ! ping -c 5 google.com >& /dev/null; then
	echo "can't access to DNS server"
fi
EOF
		fi
	fi

	if [ -f $NET ]; then
		chmod +x $NET
		return $?
	else
		return 1
	fi
}

function generate_initrd() {
	local COUNT=0

	CURRENT=$(pwd)
	cd "$INIT_DIR/$BBOX_DIRNAME/" || error "can't cd to $INIT_DIR/$BBOX_DIRNAME"

	cp -rf _install/ initramfs/ >& /dev/null
	cp -fR examples/bootfloppy/etc initramfs >& /dev/null

	rm -f initramfs/linuxrc
	mkdir -p initramfs/{dev,proc,sys,tests} >& /dev/null
	$SU cp -a /dev/{null,console,tty,tty1,tty2,tty3,tty4} initramfs/dev/ >& /dev/null

	# @NOTE: generate our initscript which is used to call our testing system
	if [ $MODE = "isolate" ]; then
		generate_isolate_initscript $4
	elif [ $MODE = "nat" ]; then
		generate_nat_initscript $4
	elif [ $MODE = "bridge" ]; then
		generate_bridge_initscript $4
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

	if [ $COUNT != 0  ] || [ $FORCE != 0 ]; then
		# @NOTE: add footer of the initscript
		echo "sleep 80" >> initramfs/init

		if [ -f $3 ]; then
			mv $3 initramfs/network
		fi

		if [[ ${#DEBUG} -eq 0 ]]; then
			echo "poweroff -f" >> initramfs/init
		fi

		# @NOTE: okey, everything is done from host machine, from now we should
		# pack everything into a initramfs.cpio.gz. We will use it to deploy a
		# simple VM with qemu and the kernel we have done building
		exec 2>&-
		cd $INIT_DIR/$BBOX_DIRNAME/initramfs

		find . -print0 | cpio --null -ov --format=newc | gzip -9 > "$2"
		cd $INIT_DIR
		exec 2>&1
	fi

	cd "$CURRENT" || error "can't cd to $CURRENT"
}

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
	local PROJECT=$1

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

function boot_kernel() {
	RAM_FILENAME=$1
	KER_FILENAME=$2

	mkdir -p /tmp/vms

	if [ -f "$RAM_FILENAME" ]; then
		if [ ! -f $KER_FILENAME ]; then
			compile_linux_kernel
		fi

		if [[ ${#DEBUG} -gt 0 ]]; then
			TIMEOUT="timeout 2700"
		else
			TIMEOUT=""
		fi

		if [[ ${#RAM} -eq 0 ]]; then
			RAM="1G"
		fi

		if [[ $3 -eq 1 ]] || [[ ${#DEBUG} -gt 0 ]]; then
			info "run the slave VM($IDX) with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			if [[ ${#CPU} -gt 0 ]]; then
				cat >> /tmp/vms/start << EOF
screen -S "vms.pid" -dm 					\
qemu-system-x86_64 -cpu $CPU -s -kernel "${KER_FILENAME}"	\
		-initrd "${RAM_FILENAME}"			\
		-nographic 					\
		-smp $(nproc) -m $RAM				\
		$NETWORK					\
		-append "console=ttyS0 loglevel=8 $KER_COMMANDS"
EOF
			else
				cat >> /tmp/vms/start << EOF
screen -S "vms.pid" -dm 				\
qemu-system-x86_64 -s -kernel "${KER_FILENAME}"		\
		-initrd "${RAM_FILENAME}"		\
		-nographic 				\
		-smp $(nproc) -m $RAM			\
		$NETWORK				\
		-append "console=ttyS0 loglevel=8 $KER_COMMANDS"
EOF
				fi
		elif [[ ${#CPU} -gt 0 ]]; then
			info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			cat >> /tmp/vms/start << EOF
echo "console log from master VM:"
$TIMEOUT qemu-system-x86_64 -cpu $CPU -s -kernel "${KER_FILENAME}"	\
	-initrd "${RAM_FILENAME}"					\
	-nographic 							\
	-smp $(nproc) -m $RAM						\
	$NETWORK							\
	-append "console=ttyS0 loglevel=8 $KER_COMMANDS"
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
		else
			info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			cat >> /tmp/vms/start << EOF
echo "[   INFO  ]: console log from master VM:"
$TIMEOUT qemu-system-x86_64 -s -kernel "${KER_FILENAME}"	\
		-initrd "${RAM_FILENAME}"			\
		-nographic 					\
		-smp $(nproc) -m $RAM				\
		$NETWORK					\
		-append "console=ttyS0 loglevel=8 $KER_COMMANDS"
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
		fi
			cat >> /tmp/vms/stop << EOF
screen -ls "vms.pid" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print \$1}' | while read s; do screen -XS \$s quit; done
rm -fr "$RAM_FILENAME"
rm -fr /tmp/vms
EOF
	else
		warning "i can't start QEmu because i don't see any approviated test suites"
		CODE=1
	fi
}

function boot_image() {
	IMG_FILENAME=$1
}

function start_vms() {
	CODE=0

	if [ $MODE = "bridge" ]; then
		# @NOTE: assign specific ip to this bridge

		if $SU ip addr add 192.168.1.1/24 dev $BRD; then
			$SU ip link set $BRD up
		fi
	fi

	troubleshoot 'start' /tmp/vms
	if [ -f /tmp/startvm ]; then
		if [ -f $WORKSPACE/Tests/Pipeline/Test.sh ]; then
			$WORKSPACE/Tests/Pipeline/Test.sh

			if [[ $CODE -eq 0 ]]; then
				CODE=$?
			fi
		else
			warning "without script $WORKSPACE/Tests/Pipeline/Test.sh we can't deduce the project is worked or not"
		fi
	fi

	troubleshoot 'end' /tmp/vms
	return $CODE
}

function create_image() {
	KER_FILENAME="$INIT_DIR/$KERNEL_DIRNAME/arch/x86_64/boot/bzImage"
	RAM_FILENAME="$INIT_DIR/initramfs-$1.cpio.gz"
	IFUP_FILENAME="/tmp/ifup-$1"
	IPDOWN_FILENAME="/tmp/ifdown-$1"

	$WORKSPACE/Tests/Pipeline/Build.sh $2 1 $1

	if [ $? != 0 ]; then
		warning "Fail repo $REPO/$BRANCH"
		CODE=1
	else
		if [[ $1 -eq $VMS_NUMBER ]] || [[ -f $WORKSPACE/Tests/Pipeline/Test.sh ]]; then
			MASTER=0
		else
			MASTER=1
		fi

		if [ -f $WORKSPACE/.environment ]; then
			source $WORKSPACE/.environment
		fi

		if ! generate_netscript $IDX $IFUP_FILENAME $IPDOWN_FILENAME; then
			warning "can't build network to VM($IDX)"
		elif [[ ${#IMG_FILENAME} -eq 0 ]]; then
			# @NOTE: build a new initrd

			if [ ! -f $RAM_FILENAME ]; then
				compile_busybox
			fi

			if [ ! -f $RAM_FILENAME ]; then
				generate_initrd "$WORKSPACE/build" $RAM_FILENAME $IFUP_FILENAME $IDX
			fi
		fi

		cd "$WORKSPACE" || error "can't cd to $WORKSPACE"

		if [ ${#KER_FILENAME} -gt 0 ]; then
			boot_kernel $RAM_FILENAME $KER_FILENAME $MASTER
		elif [ ${#IMG_FILENAME} -gt 0 ]; then
			boot_image $IMG_FILENAME
		fi
	fi
}

function process() {
	git submodule update --init --recursive
	
	if [ $METHOD != "build" ]; then
		return 0
	fi

	BASE="$(detect_libbase $WORKSPACE)"
	CPU=$HOST
	CODE=0
	DEBUG=""
	FORCE=0

	mkdir -p "$WORKSPACE/build"
	cd $WORKSPACE || error "can't cd to $WORKSPACE"

	# @NOTE: if we found ./Tests/Pipeline/prepare.sh, it can prove
	# that we are using Eevee as the tool to deploy this Repo
	if [ -e "$BASE/Tests/Pipeline/Prepare.sh" ]; then
		VMS_NUMBER=1

		$BASE/Tests/Pipeline/Prepare.sh
		if [ -f $WORKSPACE/.environment ]; then
			source $WORKSPACE/.environment
		fi

		if [[ ${#DEBUG} -gt 0 ]]; then
			$PIPELINE/../../Tools/Utilities/ngrok.sh ssh $DEBUG rootroot 0 22
		fi

		if [ "$MODE" = "bridge" ]; then
			$SU ip addr flush dev $BRD
		fi

		if [ $? != 0 ]; then
			warning "Fail repo $REPO/$BRANCH"
			CODE=1
		else
			for IDX in $(seq 1 1 $VMS_NUMBER); do
				create_image $IDX $1
			done
			
			start_vms
		fi
	else
		warning "repo $REPO don't support usual CI method"
		CODE=1
	fi

	rm -fr "$WORKSPACE"
	return $CODE
}

ETH="$(get_internet_interface)"

# @NOTE: activate ip forwarding
echo 1 | $SU tee -a /proc/sys/net/ipv4/ip_forward >& /dev/null

if [ "$MODE" = "bridge" ]; then
	# @NOTE: create a new vswitch
	BRD=$(get_new_bridge "brd")

	$SU iptables -t nat -A POSTROUTING -o $ETH -j MASQUERADE
	if create_bridge $BRD; then
		# @NOTE: redirect network between the bridge and the internet interface
		$SU iptables -A FORWARD -i $BRD -o $ETH -j ACCEPT	
		$SU iptables -A FORWARD -i $ETH -o $BRD -m state --state ESTABLISHED,RELATED \
				                -j ACCEPT

		# @NOTE: we will use this tool to troubleshoot the network issues
		install_package tcpdump
	else
		error "can't create $BRD"
	fi
fi

if [ "$METHOD" == "reproduce" ]; then
	if ! echo "$2" | grep ".reproduce.d" >& /dev/null; then
		if [ -e  $PIPELINE/Environment.sh ]; then
			rm -fr  $PIPELINE/Environment.sh
		fi
		
		# @NOTE: create a shortcut to connect directly with this environment
		ln -s $0 $PIPELINE/Environment.sh

		# @NOTE: okey do reproducing now since we have enough information to do right now
		$PIPELINE/Reproduce.sh
		exit $?
	fi
elif [ "$METHOD" = "prepare" ]; then
	git submodule update --init --recursive
	if ! init; then
		error "can't prepare global network system"
	fi

	WORKSPACE=$2
	BASE="$(detect_libbase $WORKSPACE)"
	mkdir -p "$WORKSPACE/build"

	# @NOTE: if we found ./Tests/Pipeline/prepare.sh, it can prove
	# that we are using Eevee as the tool to deploy this Repo
	if [ -e "$BASE/Tests/Pipeline/Prepare.sh" ]; then
		VMS_NUMBER=1

		$BASE/Tests/Pipeline/Prepare.sh
		if [ -f $WORKSPACE/.environment ]; then
			source $WORKSPACE/.environment
		fi

		if [[ ${#DEBUG} -gt 0 ]]; then
			$PIPELINE/../../Tools/Utilities/ngrok.sh ssh $DEBUG rootroot 0 22
		fi

		if [ "$MODE" = "bridge" ]; then
			$SU ip addr flush dev $BRD
		fi

		if [ $? != 0 ]; then
			warning "Fail repo $REPO/$BRANCH"
			CODE=1
		else
			for IDX in $(seq 1 1 $VMS_NUMBER); do
				if [ $BASE = $WORKSPACE ]; then
					create_image $IDX Base
				else
					create_image $IDX $(basename $WORKSPACE)
				fi

				if [ $? != 0 ]; then
					error "Fail repo $REPO/$BRANCH"
				fi
			done
		fi
	else
		error "we can't support this kind of project for reproducing automatically"
	fi
elif [ "$METHOD" = "test" ]; then
	WORKSPACE=$(pwd)

	if [ -x $PIPELINE/Test.sh ]; then
		start_vms
		CODE=$?
	else
		error "Reproduce requires script $PIPELINE/Test.sh"
	fi

	exit $CODE
elif [ "$METHOD" = "inject" ]; then
	SCREEN=$2

	if [ ! -d $PIPELINE/Plugins ]; then
		exit 0
	elif [ ! -d $PIPELINE/Plugins/QEmu ]; then
		exit 0
	fi

	for PLUGIN in $PIPELINE/Plugins/QEmu/*.sh; do
		if [ ! -x $PLUGIN ]; then
			continue
		elif ! $PLUGIN reset; then
			continue
		fi

		cat > /tmp/$(basename $PLUGIN).conf << EOF
logfile /tmp/$(basename $SCRIPT).log
logfile flush 1
log on
logtstamp after 1
	logtstamp string \"[ %t: %Y-%m-%d %c:%s ]\012\"
logtstamp on
EOF

		screen -c /tmp/$(basename $PLUGIN).conf -L -S $SCREEN -md $PLUGIN
	done
elif [ "$METHOD" = "report" ]; then
	BASE="$(detect_libbase $(pwd))"

	info "this is the log from the problematic case that we have reproduced:"
	echo "$1"
	echo ""

	if [ ! -d $PIPELINE/Plugins ]; then
		exit 0
	elif [ ! -d $PIPELINE/Plugins/QEmu ]; then
		exit 0
	fi

	for PLUGIN in $PIPELINE/Plugins/QEmu/*.sh; do
		info "this is the log from plugin $(basename $PLUGIN):"
		$PLUGIN 'report'
		echo ""
		echo ""
	done
elif [ "$METHOD" == "build" ]; then
	if [ -f './repo.list' ]; then
		info "use default ./repo.list"
	elif [[ $# -gt 3 ]]; then
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

	HOST=""

	if ! $SU grep vmx /proc/cpuinfo; then
		if ! $SU modprobe kvm; then
			HOST="host"
		fi
	fi

	cat './repo.list' | while read DEFINE; do
		SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
		REPO=${SPLITED[0]}
		BRANCH=${SPLITED[1]}
		PROJECT=$(basename $REPO)
		PROJECT=${PROJECT%.*}

		if [[ ${#SPLITED[@]} -gt 2 ]]; then
			AUTH=${SPLITED[2]}
		fi

		# @NOTE: clone this Repo, including its submodules
		git clone --branch $BRANCH $REPO $PROJECT

		if [ $? != 0 ]; then
			FAIL=(${FAIL[@]} $PROJECT)
			continue
		fi

		ROOT=$(pwd)
		WORKSPACE="$ROOT/$PROJECT"

		# @NOTE: everything was okey from now, run CI steps
		cd $WORKSPACE

		# @NOTE: detect an authenticate key of gerrit so we will use it
		# to detect which patch set should be used to build
		if [[ ${#AUTH} -gt 0 ]]; then
			if [[ ${#SPLITED[@]} -gt 3 ]]; then
				COMMIT=${SPLITED[3]}
			else
				COMMIT=$(git rev-parse HEAD)
			fi

			GERRIT=$(python -c """
uri = '$(git remote get-url --all origin)'.split('/')[2]
gerrit = uri.split('@')[-1].split(':')[0]

print(gerrit)
""")

			RESP=$(curl -s --request GET -u $AUTH "https://$GERRIT/a/changes/?q=$COMMIT" | cut -d "'" -f 2)

			if [[ ${#RESP} -eq 0 ]]; then
				FAIL=(${FAIL[@]} $PROJECT)
				continue
			fi

			REVISIONs=($(echo $RESP | python -c """
import json
import sys

try:
	for  resp in json.load(sys.stdin):
		if resp['branch'] != '$BRANCH':
			continue

		for commit in resp['revisions']:
			revision = resp['revisions'][commit]

			for parent in revision['commit']['parents']:
				if parent['commit'] == '$COMMIT':
					print(revision['ref'])
					break
except Exception as error:
	print(error)
	sys.exit(-1)
"""))

			if [ $? != 0 ]; then
				FAIL=(${FAIL[@]} $PROJECT)
				continue
			fi

			for REVIS in "${REVISIONs[@]}"; do
				info "The patch-set $REVIS base on $COMMIT"

				# @NOTE: checkout the latest patch-set for testing only
				git fetch $(git remote get-url --all origin) $REVIS
				git checkout FETCH_HEAD

				if ! process $PROJECT; then
					echo "$PROJECT" > ./fail
				fi
			done
		else
			if ! process $PROJECT; then
				echo "$PROJECT" > ./fail
			fi
		fi

	done

	rm -fr "$INIT_DIR/$BBOX_DIRNAME"
	rm -fr "$INIT_DIR/$KERNEL_DIRNAME"

	if [ -f /tmp/fail ]; then
		rm -fr /tmp/fail
		exit -1
	else
		exit 0
	fi
fi
