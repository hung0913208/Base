#!/bin/bash
# - File: reproduce.sh
# - Description: This bash script will be used to provide steps to
# reproduce any issue from scratch

PIPELINE="$(dirname "$0")"
source $PIPELINE/Logcat.sh
source $PIPELINE/Package.sh

SCRIPT="$(basename "$0")"
ISSUE=$2
ROOT=$3

if [ "$1" = "clone" ]; then
	REPO=$ROOT
	ROOT=$4
	SPEC=$5

	if [ ! -d "$ROOT/.reproduce.d" ]; then
		mkdir -p "$ROOT/.reproduce.d"
	fi

	git clone "$REPO" "$ROOT/.reproduce.d/$ISSUE"

	if [ $# -gt 3 ]; then
		cd "$ROOT/.reproduce.d/$ISSUE" || error "can't cd $ROOT/.reproduce.d/$ISSUE"

		if [[ $(grep "$SPEC" <<< $(git branch -a)) ]]; then
			git checkout -b $SPEC "origin/$SPEC"
		else
			git fetch --all
			git pull --all
			git checkout -b $(git name-rev --name-only $SPEC) $SPEC
		fi
		CODE=$?

		cd "$CURRENT" || error "can't cd $CURRENT"
		if [ "$CODE" != 0 ]; then
			error "fail to clone $ISSUE with spec $SPEC"
		fi
	fi

	if [ $? != 0 ]; then
		error "fail to clone $ISSUE"
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh" ]; then
		info "found script Fetch.sh, halt instead of invoking Create.sh"
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Create.sh" ]; then
		"$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Create.sh" "reproduce" "$ROOT/.reproduce.d/$ISSUE"

		if [ $? != 0 ]; then
			error "can't perform script create.sh to generate reproducing scripts"
		fi
	fi
elif [ "$1" = "prepare" ]; then
	CURRENT=$(pwd)

	if [ -d "$ROOT/.reproduce.d/$ISSUE/.requirement.d" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.requirement.d/$SYS" | while read PACKAGE; do
			install_package $PACKAGE

			if [ $? != 0 ]; then
				exit $?
			fi
		done
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/.requirement" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.requirement" | while read PACKAGE; do
			install_package $PACKAGE

			if [ $? != 0 ]; then
				exit $?
			fi
		done
	fi

	if [ -d "$ROOT/.reproduce.d/$ISSUE/.recompile.d" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.recompile.d/$SYS" | while read DEFINE; do
			if [[ ${#DEFINE} -gt 0 ]]; then
				SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
				REPO=${SPLITED[0]}
				BACKGROUND=${SPLITED[1]}

				info "recompile $REPO now, $BACKGROUND background"
				cd "$ROOT/.reproduce.d/$ISSUE" || error "can't cd to $ROOT/.reproduce.d/$ISSUE"


				if [ "$BACKGROUND" == "show" ]; then
					$PIPELINE/Install.sh $DEFINE
				else
					$PIPELINE/Install.sh $DEFINE &> /dev/null
				fi
				CODE=$?

				cd $CURRENT
				if [ $CODE != 0 ]; then
					error "fail recompile $DEFINE"
				fi
			fi
		done
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/.recompile" ]; then
		cat "$ROOT/.reproduce.d/$ISSUE/.recompile" | while read DEFINE; do
			if [[ ${#DEFINE} -gt 0 ]]; then
				SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
				REPO=${SPLITED[0]}
				BACKGROUND=${SPLITED[1]}

				info "recompile $REPO now, $BACKGROUND background"
				cd "$ROOT/.reproduce.d/$ISSUE" || error "can't cd to $ROOT/.reproduce.d/$ISSUE"

				if [ $BACKGROUND == "show" ]; then
					$PIPELINE/Install.sh $DEFINE
				else
					$PIPELINE/Install.sh $DEFINE &> /dev/null
				fi
				CODE=$?

				cd $CURRENT
				if [ $CODE != 0 ]; then
					error "fail recompile $DEFINE"
				fi
			fi
		done
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/fetch.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/fetch.sh "$ROOT/.reproduce.d/$ISSUE"
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/fetch.sh"
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh "$ROOT/.reproduce.d/$ISSUE"
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh"
		fi
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/install.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/install.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/install.sh"
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Install.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Install.sh

		if [ $? != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Install.sh"
		fi
	fi
elif [ "$1" = "reproduce" ]; then
	if [ -f "$ROOT/.reproduce.d/$ISSUE/test.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/test.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		else
			exit 1 # <--- try to redo again and again
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Test.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Test.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit 0
		else
			exit 1 # <--- try to redo again and again
		fi
	fi
	exit 0
elif [ "$1" = "verify" ]; then
	LOGS=$4
	CODE=$5

	if [ "$CODE" != 0 ] && [ "$CODE" != 1 ]; then
		exit $CODE
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/verify.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/verify.sh $LOGS/$ISSUE
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Verify.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Verify.sh $LOGS
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	fi
	exit 0
elif [ "$1" = "inject" ]; then
	CURRENT=$(pwd)
	screen -ls "reproduce" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print $1}' | while read s; do screen -XS $s quit; done

	cat > /tmp/Inject.conf << EOF
logfile /tmp/Inject.log
logfile flush 1
log on
logtstamp after 1
	logtstamp string \"[ %t: %Y-%m-%d %c:%s ]\012\"
logtstamp on
EOF
	if [ $4 != 0 ] && [ $4 != 1 ]; then
		exit $4
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/inject.sh" ]; then
		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		screen -c /tmp/Inject.conf -L -S "reproduce" -md $ROOT/.reproduce.d/$ISSUE/inject.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject.sh" ]; then
		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		screen -c /tmp/Inject.conf -L -S "reproduce" -md $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	elif [ -d "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject" ]; then
		for SCRIPT in $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject/*.sh; do
			cat > /tmp/$(basename $SCRIPT).conf << EOF
logfile /tmp/$(basename $SCRIPT).log
logfile flush 1
log on
logtstamp after 1
	logtstamp string \"[ %t: %Y-%m-%d %c:%s ]\012\"
logtstamp on
EOF
			screen -c /tmp/$(basename $SCRIPT).conf -L -S "reproduce" -md "$SCRIPT"
			CODE=$?

			if [ $CODE != 0 ]; then
				break
			fi
		done

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	fi
	exit 0
else
	error "no support $1"
fi

info "Congratulation, you have done step $1"

