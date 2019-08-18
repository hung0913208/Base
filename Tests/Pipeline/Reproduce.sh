#!/bin/bash
# - File: reproduce.sh
# - Description: this file will run separately to provide the way to reproduce any issues

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"
ROOT="$(git rev-parse --show-toplevel)"
CODE=0

install_package screen

if [[ ${#HOOK} -gt 0 ]]; then
	printf "$HOOK" >> ./HOOK
	$SU chmod +x ./HOOK

	if [ $? != 0 ]; then
		source ./HOOK
	fi
fi

if [[ ${#JOB} -gt 0 ]]; then
	echo "JOB: $JOB"

	if [[ "$JOB" == "build" ]]; then
		$PIPELINE/Create.sh
		exit $?
	elif [[ "$JOB" != "reproduce" ]]; then
		exit 0
	fi
fi

if [[ $# -gt 1 ]]; then
	echo "$1 $2 $3 $4 $5 $6 $7 $8" >> './repo.list'
elif [ ! -f "$ROOT/repo.list" ]; then
	if [ $# = 1 ]; then
		curl -k --insecure $REPO -o './repo.list' >> /dev/null

		if [ $? != 0 ]; then
			error "Can't fetch list of project from $REPO"
		fi
	elif [ ! -f './repo.list' ]; then
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
		STEP="${SPLITED[1]}"
		ISSUE="${SPLITED[0]}"
		REPO="${SPLITED[2]}"
		SPEC="${SPLITED[3]}"
		EMAIL="${SPLITED[4]}"
		AUTH="${SPLITED[5]}"
		COMMIT="${SPLITED[6]}"
		REVS="${SPLITED[7]}"
		IDX=0
		CODE=0

		"$PIPELINE/Libraries/Reproduce.sh" clone "$ISSUE" "$REPO" "$ROOT" "$SPEC" "$AUTH" "$COMMIT" "$REVS"
		if [ $? != 0 ]; then
			CODE=$?
			continue
		elif [ -d "$ROOT/.reproduce.d/$ISSUE" ]; then
			FOUND=0
			
			if ! "$PIPELINE/Libraries/Reproduce.sh" prepare "$ISSUE" "$ROOT"; then
				warning "Fail on preparing the issue $ISSUE"
				continue
			fi

			while [[ $IDX -lt $STEP ]] || [[  $STEP -lt 0 ]]; do
				info "reproducing turn $IDX" | tr -d '\n'

				if [ $FOUND = 0 ]; then
					rm -fr "$LOG/$ISSUE"
					touch "$LOG/$ISSUE"
				fi

				"$PIPELINE/Libraries/Reproduce.sh" inject "$ISSUE" "$ROOT" "$CODE"
				if [ $? != 0 ]; then
					error "fait to inject reproducing scripts"
				fi

				"$PIPELINE/Libraries/Reproduce.sh" reproduce "$ISSUE" "$ROOT" &> "$LOG/$ISSUE"
				CODE=$?

				if [ $CODE != 1 ]; then
					"$PIPELINE/Libraries/Reproduce.sh" verify "$ISSUE" "$ROOT" "$LOG/$ISSUE" "$CODE"

					if [ $? != 0 ]; then
						FOUND=1
						break
					else
						echo " -> fail"
					fi
				else
					echo " -> fail"
				fi

				IDX=$((IDX+1))
			done

			if [ $FOUND != 0 ]; then
				echo " -> success. Here is the log"
				echo ""

				cat "$LOG/$ISSUE"
				echo "---------------------------------------------------------------------------------"
				echo ""
				echo ""

				if [[ $EMAIL =~ 'ftp://' ]]; then
					RPATH=$(python -c "print(\"/\".join(\"$REVIEW\".split('/')[3:]))")
					HOST=$(python -c "print(\"$REVIEW\".split('@')[1].split('/')[0])")
					USER=$(python -c "print(\"$REVIEW\".split('/')[2].split(':')[0])")
					PASSWORD=$(python -c "print(\"$REVIEW\".split('/')[2].split(':')[1].split('@')[0])")
	
					lftp <<EOF
open $HOST
user $USER $PASSWORD
rmdir -f $RPATH/$ISSUE
EOF

					# @NOTE: update the new code coverage
					if ncftpput -DD -R -v -u "$USER" -p "$PASSWORD" "$HOST" "$RPATH" "$LOG/$ISSUE"; then
						exit $?
					fi
				else
					$PIPELINE/../../Tools/Utilities/fsend.sh upload "$LOG/$ISSUE" "$EMAIL"
				fi
	
				if [ ! -e "$ROOT/BUG" ]; then
					touch "$ROOT/BUG"
				fi
			fi

			$PIPELINE/Libraries/Reproduce.sh report "$ISSUE" "$LOG/$ISSUE"
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

