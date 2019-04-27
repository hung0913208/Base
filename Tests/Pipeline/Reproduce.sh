#!/bin/bash
# - File: reproduce.sh
# - Description: this file will run separately to provide the way to reproduce any issues

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

if [ ! -f "$ROOT/repo.list" ]; then
	if [ $# = 1 ]; then
		curl -k --insecure $REPO -o './repo.list' >> /dev/null

		if [ $? != 0 ]; then
			error "Can't fetch list of project from $REPO"
		fi
	else
		error "Please provide issues list before doing anything"
	fi
fi

# @NOTE: okey now, reproducing is comming
if [ -f "$PIPELINE/Libraries/Reproduce.sh" ]; then
	LOG="$ROOT/Logs"

	if [ ! -d $LOG ]; then
		mkdir -p $LOG
	fi

	cat "$ROOT/repo.list" | while read -r DEFINE; do
		SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
		STEP=${SPLITED[1]}
		ISSUE=${SPLITED[0]}
		REPO=${SPLITED[2]}

		"$PIPELINE/Libraries/Reproduce.sh" prepare "$ISSUE" "$REPO"
		if [ $? != 0 ]; then
			continue
		elif [ -d "$ROOT/.reproduce.d/$ISSUE" ]; then
			FOUND=0
			"$PIPELINE/Libraries/Reproduce.sh" prepare "$ISSUE" "$ROOT"

			if [ $? != 0 ]; then
				FOUND=1
			fi

			for _ in 0..$STEP; do
				if [ $FOUND != 0 ]; then
					break
				fi

				"$PIPELINE/Libraries/Reproduce.sh" reproduce "$ISSUE" "$ROOT" >& "$LOG/$ISSUE"
				CODE=$?

				if [ $CODE != 1 ]; then
					"$PIPELINE/Libraries/Reproduce.sh" verify "$ISSUE" "$ROOT" "$LOG/$ISSUE"

					if [ $? != 0 ]; then
						FOUND=1
					fi
				fi
			done

			if [ $FOUND != 0 ]; then
				warning "reproduce success the issue $ISSUE"

				if [ ! -e "$ROOT/BUG" ]; then
					touch "$ROOT/BUG"
				fi
			fi
		else
			warning "Fail on preparing the issue $ISSUE"
		fi
	done
fi

if [ ! -e "$ROOT/BUG" ]; then
	info "It seems the issues have been fixed completely"
else
	exit -1
fi

