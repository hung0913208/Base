#!/bin/bash
# - File: package.sh
# - Description: this file should only use with source because it provides
# a simple way to install a package to CI system

unameOut="$(uname -s)"
case "${unameOut}" in
	Linux*)	        machine=Linux;;
	Darwin*)        machine=Mac;;
	CYGWIN*)        machine=Cygwin;;
	MINGW*)	        machine=MinGw;;
	FreeBSD*)       machine=FreeBSD;;
	*)              machine="UNKNOWN:${unameOut}"
esac

if [ "$machine" == "Linux" ] || [ "$machine" == "FreeBSD" ]; then
	SU=""
	SYS=""
	INSTALL=""

	# @NOTE: check root if we didn"t have root permission
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

	# @NOTE: check and set INSTALL tool
	if which apt-get >& /dev/null; then
		SYS="debian"
		INSTALL="apt-get install -y"
		REMOVE="apt-get remove -y"
		CLEAR="apt-get autoremove -y"
	elif which apt >& /dev/null; then
		SYS="debian"
		INSTALL="apt install -y"
		REMOVE="apt remove -y"
		CLEAR="apt autoremove -y"
	elif which yum >& /dev/null; then
		SYS="redhat"
		INSTALL="yum install -y"
		REMOVE="yum remove -y"
		CLEAR="yum autoremove -y"
	elif which zypper >& /dev/null; then
		SYS="suse"
		INSTALL="zypper install -y"
		REMOVE="zypper rm --clean-deps -y"
		CLEAR="echo 'Do nothing'"
	elif which pkg >& /dev/null; then
		SYS="freebsd"
		INSTALL="pkg"
		REMOVE="pkg delete -y"
		CLEAR="pkg autoremove -y"
	else
		echo ""
		echo "I dont known your distribution, please inform me if you need to support"
		echo "You can send me an email to hung0913208@gmail.com"
		echo "Thanks"
		echo "Hung"
		echo ""
		exit -1
	fi
elif [ "$machine" == "Mac" ]; then
	SU=""
	INSTALL="brew install"
	SYS="osx"

	# @NOTE: check and set INSTALL tool
	if [ ! $(which brew) ]; then
		error "Please install brew before run anything on you OSX system"
	fi
else
	echo "I don't support this machine ${machine}"
	echo "You can send me an email to hung0913208@gmail.com"
	echo "Thanks"
	echo "Hung"
	echo ""
	exit -1
fi

# @NOTE: this function helps you to install a package
function install_package() {
	info "install $1 now"
	$SU $INSTALL $1 >& /dev/null

	if [ $? != 0 ]; then
		$SU $INSTALL $1
		error "install $1 fail"
	else
		return 0
	fi
}
