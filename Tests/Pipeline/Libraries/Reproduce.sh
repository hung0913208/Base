#!/bin/sh
# - File: reproduce.sh
# - Description: This bash script will be used to provide steps to
# reproduce any issue from scratch

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Logcat.sh
source $PIPELINE/Package.sh

SCRIPT="$(basename "$0")"

ISSUE=$2
ROOT=$3

if [[ "$1" == "clone" ]]; then
	if [ ! -d "$ROOT/.reproduce.d" ]; then
		mkdir -p "$ROOT/.reproduce.d"
	fi

	git clone "$ROOT" "$ROOT/.reproduce.d/$ISSUE"
	if [ $? != 0 ]; then
		error "fail to clone $ISSUE"
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/create.sh" ]; then
		"$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/create.sh" "reproduce" "$ROOT/.reproduce.d/$ISSUE"

		if [ $? != 0 ]; then
			error "can't perform script create.sh to generate reproducing scripts"
		fi
	fi
elif [[ "$1" == "prepare" ]]; then
	if [ -d "$ROOT/.reproduce.d/$ISSUE/.recompile.d" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.recompile.d/$SYS" | while read REPO; do
			$PIPELINE/Libraries/Install.sh $REPO

			if [ $? != 0 ]; then
				error "fail when run command "$PIPELINE/Libraries/Install.sh $REPO""
			fi
		done
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/.recompile" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.recompile" | while read REPO; do
			$PIPELINE/Libraries/Install.sh $REPO

			if [ $? != 0 ]; then
				error "fail when run command "$PIPELINE/Libraries/Install.sh $REPO""
			fi
		done
	fi

	if [ -d "$ROOT/.reproduce.d/$ISSUE/.requirement.d" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.requirement.d/$SYS" | while read REPO; do
			sudo $INSTALL $PACKAGE

			if [ $? != 0 ]; then
				error "fail when run command "sudo $INSTALL $PACKAGE""
			fi
		done
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/.requirement" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.requirement" | while read PACKAGE; do
			sudo $INSTALL $PACKAGE

			if [ $? != 0 ]; then
				error "fail when run command "sudo $INSTALL $PACKAGE""
			fi
		done
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/fetch.sh" ]; then
		$ROOT/.reproduce.d/$ISSUE/fetch.sh "$ROOT/.reproduce.d/$ISSUE"

		if [ $? != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/fetch.sh"
		fi
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/install.sh" ]; then
		$ROOT/.reproduce.d/$ISSUE/install.sh

		if [ $? != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/install.sh"
		fi
	fi
elif [[ "$1" == "reproduce" ]]; then
	if [ -f "$ROOT/.reproduce.d/$ISSUE/test.sh" ]; then
		$ROOT/.reproduce.d/$ISSUE/test.sh
		CODE=$?

		if [ $CODE != 0 ]; then
			exit -1
		else
			exit 1 # <--- try to redo again and again
		fi
	fi
elif [[ "$1" == "verify" ]]; then
	LOGS=$4
	CODE=$5

	if [ $CODE != 0 ] && [ $CODE != 1 ]; then
		exit $CODE
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/verify.sh" ]; then
		$ROOT/.reproduce.d/$ISSUE/verify.sh $LOGS/$ISSUE
		CODE=$?

		if [ $CODE != 0 ]; then
			exit -1
		fi
	fi
else
	error "no support $1"
fi

info "Congratulation, you have done step $1"
