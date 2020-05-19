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
KERNEL_DIRNAME=$(basename $KERNEL_NAME ".git")

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

		snift $EBRD $ROOT 1
		snift $IBRD $ROOT 1

		for IF_PATH in /sys/class/net/*; do
			if [ $(basename $IF_PATH) = $ETH ]; then
				snift $(basename $IF_PATH) $ROOT 0 'net net 192.168.100.0/24'
			else
				snift $(basename $IF_PATH) $ROOT
			fi
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

		if [ -f $ROOT/tcpdump.pid ]; then
			cat $ROOT/tcpdump.pid | while read PID; do
				$SU kill -15 $PID >& /dev/null
			done

			for FILE in $ROOT/dump/*.tcap; do
				if [[ $(wc -c < $FILE) -gt 0 ]]; then
					info "tcpdump $(basename $FILE) from host view:"
					cat $FILE
					echo ""
				fi
			done

			rm -fr $ROOT/tcpdump.pid
			rm -fr $ROOT/dump
		fi

		if which iptables >& /dev/null; then
			info "iptables rule from host view:"
			$SU iptables -L
			$SU iptables -t nat --list-rules
			echo ""
		fi

		if which ip >& /dev/null; then
			info "network configuration from host view:"
			$SU ip addr show
			echo ""
		else
			info "network configuration from host view:"
			$SU ifconfig
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

		if which netstat >& /dev/null; then
			info "netstat table from host view:"
			$SU netstat -tuanlp
			echo ""
		elif which ss >& /dev/null; then
			info "ss table from host view:"
			$SU ss -tuanlp
			echo ""
		fi

		if [ -f $ROOT/dump/dhcpd.log ]; then
			info "dump the dhcpd history log:"
			cat $ROOT/dump/dhcpd.log
			echo ""
		fi

		if [ -d $SCRIPT ]; then
			bash $SCRIPT/stop
		else
			bash $SCRIPT stop
		fi
		stop_dhcpd
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

if [ -f /config/modules/start ]; then
	sh /config/modules/start
fi

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

if [ -f /config/modules/start ]; then
	sh /config/modules/start
fi

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
		NETWORK=''

		# @NOTE: set ip address of this TAP interface
		ITAP="$(get_new_bridge "tap")"

		if ! create_tuntap "$ITAP"; then
			error "can't create a new tap interface $ITAP"
		fi

		# @NOTE: set ip address of this TAP interface
		ETAP="$(get_new_bridge "tap")"
		if ! create_tuntap "$ETAP"; then
			error "can't create a new tap interface $ETAP"
		fi

		$SU ip link set dev $ITAP master $IBRD
		$SU ip link set $ITAP up
		$SU ip link set dev $ETAP master $EBRD
		$SU ip link set $ETAP up

		NETWORK="$NETWORK -device e1000,netdev=network_0_$IDX,mac=$(get_new_macaddr) -netdev tap,ifname=$ITAP,id=network_0_$IDX,script=no,downscript=no"
		NETWORK="$NETWORK -device e1000,netdev=network_1_$IDX,mac=$(get_new_macaddr) -netdev tap,ifname=$ETAP,id=network_1_$IDX,script=no,downscript=no"

		cat > $CLOSE << EOF
#!/bin/sh

$SU ip addr flush dev $ITAP
$SU ip link set $ITAP down
$SU ip addr flush dev $ETAP
$SU ip link set $ETAP down
EOF

		cat > $NET << EOF
#!/bin/sh

# @NOTE: config nameserver
echo "nameserver 8.8.8.8" > /etc/resolv.conf

# @NOTE: config loopback interface
ifconfig lo 127.0.0.1

# @NOTE: config eth0 interface to connect to outside
touch /var/lib/misc/udhcpd.leases

if [ -f /etc/udhcpc/udhcpc-external.script ]; then
	echo "config eth1 with udhcpc"
	udhcpc -R -n -b -p /var/run/udhcpc.eth1.pid -i eth1 -s /etc/udhcpc/udhcpc-external.script
fi

if [ -f /etc/udhcpd.conf ]; then
	echo "start udhcpd daemon"
	udhcpd /etc/udhcpd.conf
fi

if [ -f /etc/udhcpc/udhcpc-internal.script ]; then
	echo "config eth0 with udhcpc"
	udhcpc -R -n -b -p /var/run/udhcpc.eth0.pid -i eth0 -s /etc/udhcpc/udhcpc-internal.script
fi
EOF
	fi

	if [ -f $NET ]; then
		chmod +x $NET
		return $?
	else
		return 1
	fi
}

function generate_udhcpd_internal_config() {
	cat > $1 << EOF
start 		192.168.101.4
end 		192.168.101.254
interface 	eth0
lease 	file 	/var/lib/misc/udhcpd.leases
opt 	dns 	8.8.8.8 8.8.4.4
option 	subnet 	255.255.255.0
opt 	router 	192.168.101.1
option 	lease 	864000
EOF
}

function generate_udhcpc_internal_script() {
	cat > $1 << EOF
#!/bin/sh
# udhcpc script edited by Tim Riker <Tim@Rikers.org>

RESOLV_CONF="/etc/resolv.conf"

[ -n "\$1" ] || { echo "Error: should be called from udhcpc"; exit 1; }

NETMASK=""
if command -v ip >/dev/null; then
	[ -n "\$subnet" ] && NETMASK="/\$subnet"
else
	[ -n "\$subnet" ] && NETMASK="netmask \$subnet"
fi
BROADCAST="broadcast +"
[ -n "\$broadcast" ] && BROADCAST="broadcast \$broadcast"

case "\$1" in
	deconfig)
		if command -v ip >/dev/null; then
			ip addr flush dev \$interface
			ip link set dev \$interface up
		else
			ifconfig \$interface 0.0.0.0
		fi

		if [ -f /config/internal/stop ]; then
			sh /config/internal/stop \$interface
		fi
		;;

	renew|bound)
		if command -v ip >/dev/null; then
			ip addr add \$ip\$NETMASK \$BROADCAST dev \$interface
		else
			ifconfig \$interface \$ip \$NETMASK \$BROADCAST
		fi

		if [ -f /config/internal/start ]; then
			sh /config/internal/start \$ip
		fi
		;;
esac

exit 0
EOF
	chmod +x $1
}

function generate_udhcpc_external_script() {
	cat > $1 << EOF
#!/bin/sh
# udhcpc script edited by Tim Riker <Tim@Rikers.org>

RESOLV_CONF="/etc/resolv.conf"

[ -n "\$1" ] || { echo "Error: should be called from udhcpc"; exit 1; }

NETMASK=""
if command -v ip >/dev/null; then
	[ -n "\$subnet" ] && NETMASK="/\$subnet"
else
	[ -n "\$subnet" ] && NETMASK="netmask \$subnet"
fi
BROADCAST="broadcast +"
[ -n "\$broadcast" ] && BROADCAST="broadcast \$broadcast"

case "\$1" in
	deconfig)
		if command -v ip >/dev/null; then
			ip addr flush dev \$interface
			ip link set dev \$interface up
		else
			ifconfig \$interface 0.0.0.0
		fi

		if [ -f /config/external/stop ]; then
			sh /config/external/stop \$interface
		fi
		;;

	renew|bound)
		if command -v ip >/dev/null; then
			ip addr add \$ip\$NETMASK \$BROADCAST dev \$interface
		else
			ifconfig \$interface \$ip \$NETMASK \$BROADCAST
		fi

		if [ -n "\$router" ] ; then
			while route del default gw 0.0.0.0 dev \$interface ; do
				:
			done

			metric=0
			for i in \$router ; do
				if [ "\$subnet" = "255.255.255.255" ]; then
					ip route add \$i dev \$interface
				fi

				route add default gw \$i dev \$interface metric \$((metric++))
			done
		fi

		if test -L "\$RESOLV_CONF"; then
			# If it's a dangling symlink, try to create the target.
			test -e "\$RESOLV_CONF" || touch "\$RESOLV_CONF"
		fi
		realconf=\$(readlink -f "\$RESOLV_CONF" 2>/dev/null || echo "\$RESOLV_CONF")

		tmpfile="\$realconf-\$$"
		> "\$tmpfile"

		[ -n "\$domain" ] && echo "search \$domain" >> "\$tmpfile"
		for i in \$dns ; do
			echo "nameserver \$i" >> "\$tmpfile"
		done

		mv "\$tmpfile" "\$realconf"

		if ! ping -c 5 8.8.8.8 >& /dev/null; then
			echo "can't connect to internet"
		fi

		if ! ping -c 5 google.com >& /dev/null; then
			echo "can't access to DNS server"
		fi

		if [ -f /config/external/start ]; then
			sh /config/external/start \$ip
		fi
		;;
esac

exit 0
EOF
	chmod +x $1
}

function generate_initrd() {
	local COUNT=0

	CURRENT=$(pwd)
	cd "$INIT_DIR/$BBOX_DIRNAME/" || error "can't cd to $INIT_DIR/$BBOX_DIRNAME"

	rm -fr initramfs

	cp -rf _install/ initramfs/ >& /dev/null
	cp -fR examples/bootfloppy/etc initramfs >& /dev/null

	rm -fr initramfs/linuxrc
	rm -fr initramfs/etc/udhcpd.conf

	mkdir -p initramfs/{dev,etc/udhcpc,proc,lib/modules,sys,tests,modules,var/lib/misc} >& /dev/null
	$SU cp -a /dev/{null,console,tty,tty1,tty2,tty3,tty4} initramfs/dev/ >& /dev/null

	if [ -d "$MOD_PATHNAME" ]; then
		$SU cp -a $MOD_PATHNAME initramfs/lib/modules
	fi

	if [ -d "$ETC_PATHNAME" ]; then
		$SU cp -a $ETC_PATHNAME initramfs/config
	fi

	# @NOTE: generate our initscript which is used to call our testing system
	if [ $MODE = "isolate" ]; then
		generate_isolate_initscript $4
	elif [ $MODE = "nat" ]; then
		generate_nat_initscript $4
	elif [ $MODE = "bridge" ]; then
		generate_bridge_initscript $4

		generate_udhcpd_internal_config udhcpd.conf && cp udhcpd.conf initramfs/etc/
		generate_udhcpc_internal_script udhcpc-internal.script && cp udhcpc-internal.script initramfs/etc/udhcpc/
		generate_udhcpc_external_script udhcpc-external.script && cp udhcpc-external.script  initramfs/etc/udhcpc/
	fi

	chmod a+x initramfs/init

	# @NOTE: copy only kernel modules
	for FILE in $(find $1 -type f -name '*.ko'); do
		if [ -d "$FILE" ]; then
			continue
		else
			COUNT=$(($COUNT + 1))

			cp $FILE initramfs/modules
			echo "insmod ./modules/$(basename $FILE)" >> initramfs/init
		fi
	done

	# @NOTE: copy only execuators that don't support dynamic link
	for FILE in $(find $1 -executable); do
		FILENAME=$(basename -- "$FILE")
		EXTENSION="${FILENAME##*.}"

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
			echo """
if [ -f /config/modules/stop ]; then
	sh /config/modules/stop
else
	poweroff -f
fi

""" >> initramfs/init
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
		cd "$INIT_DIR/$KERNEL_DIRNAME" && git pull origin $KERNEL_BRANCH
	else
		if ! git clone --branch $KERNEL_BRANCH $KERNEL_URL; then
			curl -k $KERNEL_URL -o $KERNEL_DIRNAME
			return $?
		fi

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

	if [ ! -e ${KER_FILENAME} ]; then
		error "can't create ${KER_FILENAME}"
	elif [[ ${#FTP} -gt 0 ]]; then
		if ! tar -czf $ROOT/$KERNEL_NAME.tar.gz -C "$INIT_DIR/$KERNEL_DIRNAME" .; then
			error "can't compress $KERNEL_NAME.tar.gz"
		fi

		if ! wput -p -B  $ROOT/$KERNEL_NAME.tar.gz "$FTP"; then
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
	elif [[ -d "${PROJECT}/libbase" ]]; then
		echo "${PROJECT}/libbase"
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
	IDX=$4

	mkdir -p $ROOT/vms

	if [ "$CPU" = 'host' ]; then
		KVM='-enable-kvm'
	fi

	if [[ $IDX -lt 10 ]]; then
		PORT="0$IDX"
	else
		PORT=$IDX
	fi

	if [ -f "$RAM_FILENAME" ]; then
		if [ ! -f $KER_FILENAME ]; then
			compile_linux_kernel
		fi

		if [[ ${#TIMEOUT} -eq 0 ]]; then
			if [[ ${#DEBUG} -gt 0 ]]; then
				TIMEOUT="timeout 2700"
			else
				TIMEOUT=""
			fi
		fi

		if [[ ${#RAM} -eq 0 ]]; then
			RAM="1G"
		fi

		if [[ $3 -eq 1 ]] || [[ ${#DEBUG} -gt 0 ]]; then
			info "run the slave VM($IDX) with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			if [[ ${#CPU} -gt 0 ]]; then
				cat >> $ROOT/vms/start << EOF
screen -S "vms.pid" -dm 						\
qemu-system-x86_64 -cpu $CPU -s -kernel "${KER_FILENAME}"		\
		-initrd "${RAM_FILENAME}"				\
		-nographic 						\
		-smp $(nproc) -m $RAM					\
		$NETWORK						\
		-serial telnet:localhost:101$PORT,server,nowait 	\
		-append "console=ttyAMA0,115200 console=tty  highres=off console=ttyS0 loglevel=8 $KER_COMMANDS" $KVM
EOF
			else
				cat >> $ROOT/vms/start << EOF
screen -S "vms.pid" -dm 						\
qemu-system-x86_64 -kernel "${KER_FILENAME}"				\
		-initrd "${RAM_FILENAME}"				\
		-nographic 						\
		-m $RAM							\
		$NETWORK						\
		-serial telnet:localhost:101$PORT,server,nowait 	\
		-append "console=ttyAMA0,115200 console=tty  highres=off console=ttyS0 loglevel=8 $KER_COMMANDS"
EOF
			fi
		elif [[ ${#CPU} -gt 0 ]]; then
			info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			cat >> $ROOT/vms/start << EOF
echo "[   INFO  ]: console log from master VM($CPU):"
$TIMEOUT qemu-system-x86_64 -cpu $CPU -s -kernel "${KER_FILENAME}"	\
	-initrd "${RAM_FILENAME}"					\
	-nographic 							\
	-smp $(nproc) -m $RAM						\
	$NETWORK							\
	-serial telnet:localhost:101$PORT,server,nowait 		\
	-append "console=ttyAMA0,115200 console=tty  highres=off console=ttyS0 loglevel=8 $KER_COMMANDS" $KVM
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
		else
			info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			cat >> $ROOT/vms/start << EOF
echo "[   INFO  ]: console log from master VM:"
$TIMEOUT qemu-system-x86_64 -s -kernel "${KER_FILENAME}"		\
		-initrd "${RAM_FILENAME}"				\
		-nographic 						\
		-m $RAM							\
		$NETWORK						\
		-serial telnet:localhost:101$PORT,server,nowait 	\
		-append "console=ttyAMA0,115200 console=tty  highres=off console=ttyAMA0,115200 console=tty  highres=off console=ttyS0 loglevel=8 $KER_COMMANDS"
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
		fi
			cat >> $ROOT/vms/stop << EOF
screen -ls "vms.pid" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print \$1}' | while read s; do screen -XS \$s quit; done
rm -fr "$RAM_FILENAME"
rm -fr $ROOT/vms
EOF
	else
		warning "i can't start QEmu because i don't see any approviated test suites"
		CODE=1
	fi
}

function boot_image() {
	IMG_FILENAME=$1
	IDX=$3

	mkdir -p $ROOT/vms

	if [ $CPU = 'host' ]; then
		KVM='-enable-kvm'
	fi

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

		if [[ $IDX -lt 10 ]]; then
			PORT="0$IDX"
		else
			PORT=$IDX
		fi

		if [[ $2 -eq 1 ]] || [[ ${#DEBUG} -gt 0 ]]; then
			info "run the slave VM($IDX) with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			if [[ ${#CPU} -gt 0 ]]; then
				if [ ${#VNC} -gt 0 ]; then
					cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 											\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio		\
		-cpu $CPU -s -hda "${IMG_FILENAME} 								\
		-smp $(nproc) -m $RAM										\
		-serial telnet:localhost:101$PORT,server,nowait 						\
		$NETWORK $KVM
EOF
				else
					cat >> $ROOT/vms/start << EOF
screen -S "vms.pid" -dm 					\
qemu-system-x86_64 -cpu $CPU -s -hda "${IMG_FILENAME}"		\
		-nographic 					\
		-smp $(nproc) -m $RAM				\
		-serial telnet:localhost:101$PORT,server,nowait \
		$NETWORK $KVM
EOF
				fi
			else
				if [ ${#VNC} -gt 0 ]; then
					cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 											\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio		\
		-hda "${IMG_FILENAME} 										\
		-m $RAM												\
		-serial telnet:localhost:101$PORT,server,nowait 						\
		$NETWORK $KVM
EOF
				else
					cat >> $ROOT/vms/start << EOF
screen -S "vms.pid" -dm 					\
qemu-system-x86_64 -serial stdio 				\
		-hda "${IMG_FILENAME}"				\
		-nographic 					\
		-m $RAM						\
		-serial telnet:localhost:101$PORT,server,nowait \
		$NETWORK $KVM
EOF
				fi
			fi
		elif [[ ${#CPU} -gt 0 ]]; then
			info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			if [ ${#VNC} -gt 0 ]; then
				cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 											\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio		\
		-cpu $CPU -s -hda "${IMG_FILENAME} 								\
		-smp $(nproc) -m $RAM										\
		-serial telnet:localhost:101$PORT,server,nowait 						\
		$NETWORK $KVM
EOF
			else
				cat >> $ROOT/vms/start << EOF
echo "[   INFO  ]: console log from master VM($CPU):"
$TIMEOUT qemu-system-x86_64 -cpu $CPU -s -hda "${IMG_FILENAME}"	\
	-nographic 						\
	-smp $(nproc) -m $RAM					\
	-serial telnet:localhost:101$PORT,server,nowait 	\
	$NETWORK $KVM
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
			fi
		else
			info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			if [ ${#VNC} -gt 0 ]; then
				cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 											\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio		\
		-hda "${IMG_FILENAME} 										\
		-m $RAM												\
		-serial telnet:localhost:101$PORT,server,nowait 						\
		$NETWORK $KVM
EOF
			else
				cat >> $ROOT/vms/start << EOF
echo "[   INFO  ]: console log from master VM:"
$TIMEOUT qemu-system-x86_64 -hda "${IMG_FILENAME}"		\
		-nographic 					\
		-m $RAM						\
		-serial telnet:localhost:101$PORT,server,nowait \
		$NETWORK $KVM
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
			fi
		fi
			cat >> $ROOT/vms/stop << EOF
screen -ls "vms.pid" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print \$1}' | while read s; do screen -XS \$s quit; done
rm -fr $ROOT/vms
EOF
	else
		warning "i can't start QEmu because i don't see any approviated test suites"
		CODE=1
	fi
}

function boot_netkernel() {
	error "no support booting kernel with nfs"
}

function boot_pxelinux() {
	IDX=$1

	mkdir -p $ROOT/vms

	if [ $CPU = 'host' ]; then
		KVM='-enable-kvm'
	fi

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

		if [[ $IDX -lt 10 ]]; then
			PORT="0$IDX"
		else
			PORT=$IDX
		fi

		if [[ $2 -eq 1 ]] || [[ ${#DEBUG} -gt 0 ]]; then
			info "run the slave VM($IDX) with pxeboot"

			if [[ ${#CPU} -gt 0 ]]; then
				if [ ${#VNC} -gt 0 ]; then
					cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 										\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio	\
		-cpu $CPU -s -boot n 									\
		-smp $(nproc) -m $RAM									\
		-serial telnet:localhost:101$PORT,server,nowait 					\
		$NETWORK $KVM
EOF
				else
					cat >> $ROOT/vms/start << EOF
screen -S "vms.pid" -dm 					\
qemu-system-x86_64 -serial stdio 				\
		-cpu $CPU -s -boot n 				\
		-nographic 					\
		-smp $(nproc) -m $RAM				\
		-serial telnet:localhost:101$PORT,server,nowait \
		$NETWORK $KVM
EOF
				fi
			else
				if [ ${#VNC} -gt 0 ]; then
					cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 											\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio		\
		-smp $(nproc) -m $RAM -boot n									\
		-serial telnet:localhost:101$PORT,server,nowait 						\
		$NETWORK $KVM
EOF
				else
					cat >> $ROOT/vms/start << EOF
screen -S "vms.pid" -dm 					\
qemu-system-x86_64 -serial stdio 				\
		-nographic -boot n 				\
		-m $RAM						\
		-serial telnet:localhost:101$PORT,server,nowait \
		$NETWORK $KVM
EOF
				fi
			fi
		elif [[ ${#CPU} -gt 0 ]]; then
			info "run the master VM with pxeboot"

			if [ ${#VNC} -gt 0 ]; then
				cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 											\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio		\
		-cpu $CPU -s -boot n 										\
		-smp $(nproc) -m $RAM										\
		-serial telnet:localhost:101$PORT,server,nowait 						\
		$NETWORK $KVM
EOF
			else
				cat >> $ROOT/vms/start << EOF
echo "[   INFO  ]: console log from master VM($CPU):"
$TIMEOUT qemu-system-x86_64 -serial stdio 		\
	-cpu $CPU -s -boot n 				\
	-nographic 					\
	-smp $(nproc) -m $RAM				\
	-serial telnet:localhost:101$PORT,server,nowait \
	$NETWORK $KVM
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
			fi
		else
			info "run the master VM with kernel $KER_FILENAME and initrd $RAM_FILENAME"

			if [ ${#VNC} -gt 0 ]; then
				cat >> $ROOT/vms/start << EOF
VNC_PW=$(echo $VNC | awk '{ split($0,a,":"); print a[1] }')
VNC_ID=$(echo $VNC | awk '{ split($0,a,":"); print a[2] }')

screen -S "vms.pid" -dm 											\
printf "change vnc password\n%s\n" VNC_PW | qemu-system-x86_64 -vnc ":\$VNC_ID,password" -monitor stdio		\
		-boot n -smp $(nproc) -m $RAM									\
		-serial telnet:localhost:101$PORT,server,nowait 						\
		$NETWORK $KVM
EOF
			else
				cat >> $ROOT/vms/start << EOF
echo "[   INFO  ]: console log from master VM:"
$TIMEOUT qemu-system-x86_64 -serial stdio 			\
		-nographic -boot n 				\
		-m $RAM						\
		-serial telnet:localhost:101$PORT,server,nowait \
		$NETWORK $KVM
echo "--------------------------------------------------------------------------------------------------------------------"
echo ""
EOF
			fi
		fi
			cat >> $ROOT/vms/stop << EOF
screen -ls "vms.pid" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print \$1}' | while read s; do screen -XS \$s quit; done
rm -fr $ROOT/vms
EOF
	else
		warning "i can't start QEmu because i don't see any approviated test suites"
		CODE=1
	fi
}

function start_vms() {
	CODE=0

	if [ $MODE = "bridge" ]; then
		# @NOTE: assign specific ip to this bridge

		if $SU ip addr add 192.168.100.1/24 dev $EBRD; then
			$SU ip link set $EBRD up
			$SU kill -15 $(pgrep dnsmasq)

			cat > $ROOT/vms/dhcpd.script << EOF
OP="\${1:-op}"
MAC="\${2:-mac}"
IP="\${3:-ip}"
HOSTNAME="\${4}"

TIMESTAMP="\$(date '+%Y-%m-%d %H:%M:%S')"
echo "\$TIMESTAMP: <\$MAC> \$OP \$HOSTNAME/\$IP" >> $ROOT/dump/dhcpd.log
EOF
			chmod +x $ROOT/vms/dhcpd.script

			if ! start_dhcpd $EBRD $ROOT/vms/dhcpd.script; then
				error "can't start dhcpd service"
			fi
		fi

		if $SU ip addr add 192.168.101.254/24 dev $IBRD; then
			$SU ip link set $IBRD up
		fi
	fi

	troubleshoot 'start' $ROOT/vms
	if [ -f $ROOT/vms/start ]; then
		if [ -f $WORKSPACE/Tests/Pipeline/Test.sh ]; then
			$WORKSPACE/Tests/Pipeline/Test.sh

			if [[ $CODE -eq 0 ]]; then
				CODE=$?
			fi
		else
			warning "without script $WORKSPACE/Tests/Pipeline/Test.sh we can't deduce the project is worked or not"
		fi
	fi

	troubleshoot 'end' $ROOT/vms
	return $CODE
}

function create_image() {
	MOD_PATHNAME=""
	KER_FILENAME="$INIT_DIR/$KERNEL_DIRNAME/arch/x86_64/boot/bzImage"
	RAM_FILENAME="initramfs.cpio.gz"
	IFUP_FILENAME="$ROOT/ifup"
	IPDOWN_FILENAME="$ROOT/ifdown"

	if ! $WORKSPACE/Tests/Pipeline/Build.sh $2 1 $1; then
		warning "Fail repo $REPO/$BRANCH"
		CODE=1
	else
		if [[ $1 -eq $VMS_NUMBER ]] || [[ -f $WORKSPACE/Tests/Pipeline/Test.sh ]]; then
			SLAVER=0
		else
			SLAVER=1
		fi

		if [ -f $WORKSPACE/.environment ]; then
			source $WORKSPACE/.environment
		fi

		if [ ! -f $RAM_FILENAME ] && [[ ${#RAM_FILENAME} -gt 0 ]]; then
			RAM_FILENAME="$INIT_DIR/$RAM_FILENAME"
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

		if [ -f $WORKSPACE/Tests/Pipeline/Boot.sh ]; then
			$WORKSPACE/Tests/Pipeline/Boot.sh $1 $SLAVER
		elif [ ${#KER_FILENAME} -gt 0 ]; then
			boot_kernel $RAM_FILENAME $KER_FILENAME $SLAVER $1
		elif [ ${#IMG_FILENAME} -gt 0 ]; then
			boot_image $IMG_FILENAME $SLAVER $1
		else
			boot_pxelinux $1 $SLAVER
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
			$SU ip addr flush dev $EBRD
			$SU ip addr flush dev $IBRD
		fi

		if [ $? != 0 ]; then
			warning "Fail repo $REPO/$BRANCH"
			CODE=1
		else
			for IDX in $(seq 1 1 $VMS_NUMBER); do
				create_image $IDX $1
			done

			if [[ $CODE -eq 0 ]]; then
				start_vms
			else
				warning "Fail repo $REPO/$BRANCH"
			fi
		fi
	else
		warning "repo $REPO don't support usual CI method"
		CODE=1
	fi

	if [ $ROOT != $WORKSPACE ]; then
		rm -fr "$WORKSPACE"
	fi
	return $CODE
}

ETH="$(get_internet_interface)"

# @NOTE: activate ip forwarding
echo 1 | $SU tee -a /proc/sys/net/ipv4/ip_forward >& /dev/null

if [ "$MODE" = "bridge" ]; then
	# @NOTE: config nat table to the internet interface to response packets from bridges
	$SU iptables -t nat -A POSTROUTING -o $ETH -j MASQUERADE

	# @NOTE: config iptables to allow bridges can response packet to them
	$SU iptables -I FORWARD -m physdev --physdev-is-bridged -j ACCEPT

	# @NOTE: create a new external bridge
	EBRD=$(get_new_bridge "brd")

	if create_bridge $EBRD; then
		# @NOTE: redirect network between the bridge and the internet interface
		$SU iptables -A FORWARD -i $EBRD -o $ETH -j ACCEPT
		$SU iptables -A FORWARD -i $ETH -o $EBRD -m state --state ESTABLISHED,RELATED \
						-j ACCEPT

		# @NOTE: we will use this tool to troubleshoot the network issues
		install_package tcpdump
	else
		error "can't create $EBRD"
	fi

	# @NOTE: create a new internal bridge
	IBRD=$(get_new_bridge "brd")

	if ! create_bridge $IBRD; then
		error "can't create $IBRD"
	fi
elif [ "$MODE" = "isolate" ]; then
	NETWORK="-netdev user,id=network0 -device e1000,netdev=network0,mac=$(get_new_macaddr)"
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
			$SU ip addr flush dev $EBRD
			$SU ip addr flush dev $IBRD
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

				if [ $CODE != 0 ]; then
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

		cat > $ROOT/$(basename $PLUGIN).conf << EOF
logfile $ROOT/$(basename $SCRIPT).log
logfile flush 1
log on
logtstamp after 1
	logtstamp string \"[ %t: %Y-%m-%d %c:%s ]\012\"
logtstamp on
EOF

		screen -c $ROOT/$(basename $PLUGIN).conf -L -S $SCREEN -md $PLUGIN
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
	elif [[ ${#REPO} -gt 0 ]]; then
		# @NOTE: fetch the list project we would like to build from remote server
		if [ ! -f './repo.list' ]; then
			curl -k --insecure $REPO -o './repo.list' >> /dev/null

			if [ $? != 0 ]; then
				error "Can't fetch list of project from $REPO"
			fi
		fi
	elif [ "$(detect_libbase $ROOT)" != "$ROOT" ]; then
		echo "$(realpath $ROOT) HEAD" > ./repo.list
	else
		error "can't find repo.list"
	fi

	HOST=""

	if $SU egrep '(vmx|svm)' /proc/cpuinfo; then
		if $SU modprobe kvm; then
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

		if [ ! -d $REPO ]; then
			git clone --branch $BRANCH $REPO $PROJECT

			if [ $? != 0 ]; then
				FAIL=(${FAIL[@]} $PROJECT)
				continue
			fi

			ROOT=$(pwd)
			WORKSPACE="$ROOT/$PROJECT"

		else
			WORKSPACE=$REPO
		fi

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

	if [ -f $ROOT/fail ]; then
		rm -fr $ROOT/fail
		exit -1
	else
		exit 0
	fi
fi
