#!/bin/bash
# - File: reproduce.sh
# - Description: this file will run separately to provide the way to reproduce any issues

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"
ROOT="$(git rev-parse --show-toplevel)"
CODE=0

function progress() {
	python -c """
import sys
number = '$1'

if len(number) > 9:
	number = number[:9]
else:
	number = ' '*(9 - len(number)) + number

sys.stdout.write('\\r[%s]: $2' % number)
sys.stdout.flush()
"""
}

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
	BEGIN=$(date +%s)
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
		INTERVIEW="${SPLITED[4]}"
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
				progress $IDX "Preproducing ->"
				
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

				if [ $CODE != 0 ]; then
					"$PIPELINE/Libraries/Reproduce.sh" verify "$ISSUE" "$ROOT" "$LOG/$ISSUE" "$CODE"

					if [ $? != 0 ]; then
						FOUND=1
						break
					else
						echo ' fail' | tr -d '\n'
					fi
				else
					echo ' fail' | tr -d '\n'
				fi

				END=$(date +%s)

				if [[ ${#TIMEOUT} -gt 0 ]] && [[ $((END-BEGIN)) -gt $TIMEOUT ]]; then
					break
				else
					IDX=$((IDX+1))
				fi
			done

			if [ $FOUND != 0 ]; then
				echo " success. Here is the log"
				echo ""

				cat "$LOG/$ISSUE"

				if [[ $INTERVIEW =~ 'ftp://' ]]; then
					RPATH=$(python -c "print(\"/\".join(\"$INTERVIEW\".split('/')[3:]))")
					HOST=$(python -c "print(\"$INTERVIEW\".split('@')[1].split('/')[0])")
					USER=$(python -c "print(\"$INTERVIEW\".split('/')[2].split(':')[0])")
					PASSWORD=$(python -c "print(\"$INTERVIEW\".split('/')[2].split(':')[1].split('@')[0])")
	
					lftp $HOST -u $USER,$PASSWORD -e "set ftp:ssl-allow no;" <<EOF
put -O $RPATH $LOG/$ISSUE
EOF

				else
					echo "---------------------------------------------------------------------------------"
					echo ""
					echo ""
					$PIPELINE/../../Tools/Utilities/fsend.sh upload "$LOG/$ISSUE" "$INTERVIEW"
				fi
	
				if [ ! -e "$ROOT/BUG" ]; then
					touch "$ROOT/BUG"
				fi
			else
				echo ''
			fi

			$PIPELINE/Libraries/Reproduce.sh report "$ISSUE" "$ROOT" "$LOG/$ISSUE"
		else
			warning "Fail on preparing the issue $ISSUE"
		fi
		
		END=$(date +%s)

		rm -fr "$ROOT/.reproduce.d/$ISSUE"
		if [[ ${#TIMEOUT} -gt 0 ]] && [[ $((END-BEGIN)) -gt $TIMEOUT ]]; then
			break
		fi
	done
fi

if [ "$CODE" != 0 ]; then
	error "Got problems during reproduce"
elif [ ! -e "$ROOT/BUG" ]; then
	info "It seems the issues have been fixed completely"
else
	exit -1
fi

