#!/bin/bash

REPO=$3
METHOD=$1
BRANCH=$4
PIPELINE=$2

source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

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
			"$0" $METHOD $PIPELINE $REPO $BRANCH
		fi

		exit $?
	fi

	cd $CURRENT || error "can't cd to $CURRENT"
fi

# @NOTE: otherwide, we should the current environment since the branch maybe
# merged with the master
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

INIT_DIR=$PIPELINE
BBOX_URL="https://busybox.net/downloads/busybox-1.30.1.tar.bz2"
BBOX_FILENAME=$(basename $BBOX_URL)
BBOX_DIRNAME=$(basename $BBOX_FILENAME ".tar.bz2")

KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.4.121.tar.gz"
KERNEL_FILENAME=$(basename $KERNEL_URL)
KERNEL_DIRNAME=$(basename $KERNEL_FILENAME ".tar.gz")

KER_FILENAME="$INIT_DIR/$KERNEL_DIRNAME/arch/x86_64/boot/bzImage"
RAM_FILENAME="$INIT_DIR/initramfs.cpio.gz"

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

function create_initrd(){
	cat > "$INIT_DIR/$BBOX_DIRNAME/initramfs/init" << EOF
#!/bin/busybox sh
mount -t proc none /proc
mount -t sysfs none /sys
echo "hello from qemu"
ifconfig -a

echo "system's going to poweroff"
sleep 1
poweroff -f
EOF
}

function generate_initrd() {
	exec 2>&-
	cd $INIT_DIR/$BBOX_DIRNAME/initramfs

	find . -print0 | cpio --null -ov --format=newc | gzip -9 > "${RAM_FILENAME}"
	cd $INIT_DIR
	exec 2>&1
}

function config_busybox() {
	cd "$INIT_DIR/$BBOX_DIRNAME/"
	cp -rf _install/ initramfs/ >& /dev/null
	rm -f initramfs/linuxrc
	mkdir -p initramfs/{dev,proc,sys} >& /dev/null
	$SU cp -a /dev/{null,console,tty,tty1,tty2,tty3,tty4} initramfs/dev/ >& /dev/null

	create_initrd
	chmod a+x initramfs/init

	generate_initrd
	cd "${INIT_DIR}/${BBOX_DIRNAME}/initramfs/"
	cd "$INIT_DIR"
}

function compile_kernel() {
	info "build a specific kernel"
	cd "$INIT_DIR"
	curl "$KERNEL_URL" -o "$KERNEL_FILENAME"
	tar xf "$KERNEL_FILENAME"

	cd "$INIT_DIR/$KERNEL_DIRNAME"
	make defconfig >& /dev/null

	# @NOTE: by default we will use default config of linux kernel but we
	# will use the specify if it's configured
	if [ -f $ROOT/.config ]; then
		cp $ROOT/.config ./.config
	fi

	make -j32
	cd "$INIT_DIR"
}

compile_busybox
config_busybox
compile_kernel

info "run qemu-system-x86 with kernel $KER_FILENAME and initrd $RAM_FILENAME"
qemu-system-x86_64 -s -kernel "${KER_FILENAME}" -initrd "${RAM_FILENAME}" -nographic \
		-netdev user,id=user.0 -device e1000,netdev=user.0 \
		-append "console=ttyS0 loglevel=8"

$PIPELINE/../../Tools/Utilities/fsend.sh upload $KER_FILENAME hung0913208@gmail.com
$PIPELINE/../../Tools/Utilities/fsend.sh upload $RAM_FILENAME hung0913208@gmail.com
