#!/bin/bash
# - File: reproduce.sh
# - Description: this file will run separately to provide the way to reproduce any issues

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"
ROOT="$(git rev-parse --show-toplevel)"
CODE=0

if [[ $# -gt 1 ]]; then
	echo "$1 $2 $3 $4 $5" >> './repo.list'
elif [ ! -f "$ROOT/repo.list" ]; then
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

	cat "./repo.list" | while read -r DEFINE; do
		SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
		STEP=${SPLITED[1]}
		ISSUE=${SPLITED[0]}
		REPO=${SPLITED[2]}
		SPEC=${SPLITED[3]}
		EMAIL=${SPLITED[4]}
		CODE=0

		"$PIPELINE/Libraries/Reproduce.sh" clone "$ISSUE" "$REPO" "$ROOT" "$SPEC"
		if [ $? != 0 ]; then
			CODE=$?
			continue
		elif [ -d "$ROOT/.reproduce.d/$ISSUE" ]; then
			FOUND=0
			"$PIPELINE/Libraries/Reproduce.sh" prepare "$ISSUE" "$ROOT"

			if [ $? != 0 ]; then
				FOUND=1
			fi

			for IDX in $(seq 0 $STEP); do
				info "Begin reproducing turn $IDX"

				if [ $FOUND = 0 ]; then
					rm -fr "$LOG/$ISSUE"
					touch "$LOG/$ISSUE"
				fi

				"$PIPELINE/Libraries/Reproduce.sh" reproduce "$ISSUE" "$ROOT" &> "$LOG/$ISSUE"
				CODE=$?

				if [ $CODE != 1 ]; then
					"$PIPELINE/Libraries/Reproduce.sh" verify "$ISSUE" "$ROOT" "$LOG/$ISSUE" "$CODE"

					if [ $? != 0 ]; then
						FOUND=1
						break
					else
						info "Fail reproducing turn $IDX"
					fi
				else
					info "Fail reproducing turn $IDX"
				fi
			done

			if [ $FOUND != 0 ]; then
				warning "reproduce success the issue $ISSUE"

				$PIPELINE/../../Tools/Utilities/fsend.sh upload "$LOG/$ISSUE" "$EMAIL"
				if [ ! -e "$ROOT/BUG" ]; then
					touch "$ROOT/BUG"
				fi
			fi

		else
			warning "Fail on preparing the issue $ISSUE"
		fi

		rm -fr "$ROOT/.reproduce.d/$ISSUE"
	done
fi

if [ "$CODE" != 0 ]; then
	error "Got problems during reproduce"
elif [ ! -e "$ROOT/BUG" ]; then
	info "It seems the issues have been fixed completely"
else
	exit -1
fi

