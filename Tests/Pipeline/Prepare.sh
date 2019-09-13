#!/bin/bash
# - File: prepare.sh
# - Description: This bash script will be run early before build.sh and test.sh
# and it will be used to define a specific environment to build any projects

# @NOTE: detect system os, we need this infomation to define our own package
# managerment
PIPELINE="$(dirname "$0")"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

# @NOTE: prepare our playground
if [[ $# -gt 0 ]]; then
	PROJECT=$1
	shift
else
	PROJECT="$(basename `git config  --get remote.origin.url`)"
fi

info "You have run on machine ${machine} script ${SCRIPT} project ${PROJECT}"
if [ "$machine" == "Linux" ] || [ "$machine" == "FreeBSD" ]; then
	# @NOTE: show memory status
	if [ $(which free) ]; then
		info "Memory of this CI:"
		free -m
		echo ""
	fi

	# @NOTE: check and set INSTALL tool
	if [ $(which apt-get) ]; then
		REQUIRED=("ssh" "python" "git" "lftp" "ncftp" "expect")
		$SU apt-get update &> /dev/null

		# @NOTE: print infomation of this device
		info "It seems your distribute is Ubuntu/Debian"
		info "Kernel info $(uname -a)"
	elif [ $(which apt) ]; then
		REQUIRED=("ssh" "python" "git" "lftp" "ncftp" "expect")
		$SU apt update &> /dev/null

		# @NOTE: print infomation of this device
		info "It seems your distribute is Ubuntu/Debian"
		info "Kernel info $(uname -a)"
	elif [ $(which yum) ]; then
		REQUIRED=("openssh" "python" "git" "lftp" "ncftp" "expect")
		$SU yum update

		# @NOTE: print infomation of this device
		info "It seems your distribute is Fedora/Centos"
		info "Kernel info $(uname -a)"
	elif [ $(which zypper) ]; then
		REQUIRED=("openssh" "python" "git" "lftp" "ncftp" "expect")
		$SU zypper update

		# @NOTE: print infomation of this device
		info "It seems your distribute is Opensuse"
		info "Kernel info $(uname -a)"
	elif [ $(which pkg) ]; then
		REQUIRED=("ssh" "python" "git" "lftp" "ncftp" "expect")
		$SU pkg update

		# @NOTE: print infomation of this device
		info "It seems your distribute is FreeBSD"
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
	REQUIRED=("ssh" "python" "git")

	# @NOTE: show memory status
	info "Memory of this CI:"
	vm_stat | perl -ne '/page size of (\d+)/ and $size=$1; /Pages\s+([^:]+)[^\d]+(\d+)/ and printf("%-16s % 16.2f Mi\n", "$1:", $2 * $size / 1048576);'
	echo ""

	# @NOTE: print infomation of this device
	info "It seems your distribute is MacOS"
else
	error "Don't support this machine ${machine}"
fi

# okey, install basic packages which are defined above
for PACKAGE in "${REQUIRED[@]}"; do
	install_package $PACKAGE
done

ROOT="$(git rev-parse --show-toplevel)"
PROJECT="$(basename `git config  --get remote.origin.url`)"

# @NOTE: now install requirement if it has been defined
info "check .requirement inside $ROOT"
if [ -d "$ROOT/.requirement.d" ]; then
	info "found $ROOT/.requirement.d -> check $SYS"

	if [ -e "$ROOT/.requirement.d/$SYS" ]; then
		cat "$ROOT/.requirement.d/$SYS" | while read DEFINE; do
			if [[ ${#DEFINE} -gt 0 ]]; then
				install_package $DEFINE
			fi
		done
	else
		warning "not found "$ROOT/.requirement.d/$SYS""
	fi
elif [ -f "$ROOT/.requirement" ]; then
	cat "$ROOT/.requirement" | while read DEFINE; do
		if [[ ${#DEFINE} -gt 0 ]]; then
			install_package $DEFINE

			if [ $? != 0 ]; then
				warning "fail install $DEFINE"
			fi
		fi
	done
else
	warning "not found $ROOT/.requirement.d/$SYS or $ROOT/.requirement"
fi

# @NOTE: now recompile and install some packages since distro may deliver wrong
info "check .recompile inside $ROOT"
if [ -d "$ROOT/.recompile.d" ]; then
	info "found $ROOT/.recompile.d -> check $SYS"

	if [ -e "$ROOT/.recompile.d/$SYS" ]; then
		cat "$ROOT/.recompile.d/$SYS" | while read DEFINE; do
			if [[ ${#DEFINE} -gt 0 ]]; then
				SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
				REPO=${SPLITED[0]}
				BACKGROUND=${SPLITED[1]}

				info "recompile $REPO now, $BACKGROUND background"

				if [[ $BACKGROUND == "show" ]]; then
					$PIPELINE/Libraries/Install.sh $REPO
				else
					$PIPELINE/Libraries/Install.sh $REPO &> /dev/null
				fi

				if [ $? != 0 ]; then
					error "fail recompile $REPO"
				fi
			fi
		done
	else
		warning "not found "$ROOT/.recompile.d/$SYS""
	fi
elif [ -f "$ROOT/.recompile" ]; then
	cat "$ROOT/.recompile" | while read DEFINE; do
		if [[ ${#DEFINE} -gt 0 ]]; then
			SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
			REPO=${SPLITED[0]}
			BACKGROUND=${SPLITED[1]}

			info "recompile $REPO now, $BACKGROUND background"

			if [ $BACKGROUND == "show" ]; then
				$PIPELINE/Libraries/Install.sh $DEFINE
			else
				$PIPELINE/Libraries/Install.sh $DEFINE &> /dev/null
			fi

			if [ $? != 0 ]; then
				error "fail recompile $DEFINE"
			fi
		fi
	done
else
	warning "not found $ROOT/.recompile.d/$SYS or $ROOT/.recompile"
fi

# @NOTE: new Tools/Builder may require preinstall some libraries from Eevee. To do
# that, i will use CMake as a convenient tool to build these libraries
if [ -f $PIPELINE/../../Tools/Builder/install.sh ]; then
	$PIPELINE/../../Tools/Builder/install.sh

	if [ $? != 0 ]; then
		error "Error when run script $ROOT/Tools/Builder/Install.sh"
	fi
fi

# @NOTE: if we use Eevee as a framework to deploy a CI, we must
# check and install extra package since depenencies may change
# dramatically
if [[ $PROJECT == "LibBase" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "LibBase.git" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "Eden" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "Eden.git" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "Base" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "Base.git" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "base" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "base.git" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "Eevee" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ $PROJECT == "Eevee.git" ]]; then
	info "Congratulation, you have passed ${SCRIPT}"
	exit 0
elif [[ -d "${ROOT}/Eden" ]]; then
	BASE="${ROOT}/Eden"
elif [[ -d "${ROOT}/Base" ]]; then
	BASE="${ROOT}/Base"
elif [[ -d "${ROOT}/base" ]]; then
	BASE="${ROOT}/base"
elif [[ -d "${ROOT}/LibBase" ]]; then
	BASE="${ROOT}/LibBase"
elif [[ -d "${ROOT}/Eevee" ]]; then
	BASE="${ROOT}/Eevee"
else
	error """
	Not found LibBase, it must be:
	- ${ROOT}/Base
	- ${ROOT}/base
	- ${ROOT}/LibBase
	- ${ROOT}/Eevee
	- ${ROOT}/Eden
	"""
fi

if [ ! -d "$ROOT/Tests" ]; then
	warning "$ROOT/Tests must exist before doing anything"

	# @NOTE: if package 'tree' has been installed, use it to present current
	# directory
	which tree &> /dev/null
	if [ $? == 0 ]; then
		tree $ROOT
	fi
else
	# @NOTE: remove $ROOT/Tests/Pipeline if it is a file
	if [ -f "$ROOT/Tests/Pipeline" ]; then
		rm -fr "$ROOT/Tests/Pipeline"
	fi

	# @NOTE: create directory $ROOT/Tests/Pipeline
	if [ ! -d "$ROOT/Tests/Pipeline" ]; then
		mkdir -p "$ROOT/Tests/Pipeline"
	fi

	# @NOTE: migrate Pipeline's libraries
	cp -a "$BASE/Tests/Pipeline/Libraries" "$ROOT/Tests/Pipeline/Libraries"

	# @NOTE: migrate Pipeline's installing scripts
	cp -a "$BASE/Tests/Pipeline/Packages" "$ROOT/Tests/Pipeline/Packages"

	# @NOTE: migrate reproduce script
	cp -a "$BASE/Tests/Pipeline/Reproduce.sh" "$ROOT/Tests/Pipeline"

	if [ ! -f $ROOT/Tests/Pipeline/Build.sh ] && [ ! -f $ROOT/Tests/Pipeline/Test.sh ]; then
		# @NOTE: migrate builder to an approviated position
		if [ -f $BASE/Tools/Builder/Install.sh ]; then
			CURRENT=$(pwd)
			cd $BASE || error "can't jump to $BASE"

			$BASE/Tools/Builder/Install.sh
			if [ $? != 0 ]; then
				error "Error when run script $BASE/Tools/Builder/Install.sh"
			fi

			cd $CURRENT || error "can't jump to $CURRENT"
		fi

		info "moving $BASE/Tests/Pipeline/*.sh -> $ROOT/Tests/Pipeline"

		# @NOTE: migrate build script if it needs
		if [ ! -f "$ROOT/Tests/Pipeline/Build.sh" ]; then
			cp -a "$BASE/Tests/Pipeline/Build.sh" "$ROOT/Tests/Pipeline"
		fi
	fi
fi

if [ -f "$ROOT/Tests/Pipeline/Prepare.sh" ]; then
	bash "$ROOT/Tests/Pipeline/Prepare.sh" $SU $INSTALL

	if [ $? != 0 ]; then
		error "Error when run script $ROOT/Tests/Prepare.sh"
	fi
fi

info "Congratulation, you have passed ${SCRIPT}"
