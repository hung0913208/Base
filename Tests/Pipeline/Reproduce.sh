#!/bin/bash
# - File: reproduce.sh
# - Description: this file will run separately to provide the way to reproduce any issues

PIPELINE="$(dirname "$0" )"

if [ -d "$PIPELINE/Libraries" ]; then
	LIBRARIES="$PIPELINE/Libraries"
elif [ -d "$PIPELINE/libraries" ]; then
	LIBRARIES="$PIPELINE/libraries"
else
	exit -1
fi

source $PIPELINE/$LIBRARIES/Logcat.sh
source $PIPELINE/$LIBRARIES/Package.sh

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
if [ -f "$PIPELINE/$LIBRARIES/Reproduce.sh" ]; then
	BEGIN=$(date +%s)
	LOG="$ROOT/Logs"
	CODE=0

	if [ ! -d $LOG ]; then
		mkdir -p $LOG
	fi

	while [ 1 ]; do
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

			if [ -f $ROOT/.reproduce.d/${ISSUE}.txt ]; then
				IDX=$(cat $ROOT/.reproduce.d/${ISSUE}.txt)
			fi
		
			if [[ $IDX -ge $STEP ]] || [[  $STEP -lt 0 ]] || [[ $IDX -lt 0 ]]; then
				rm -fr "$ROOT/.reproduce.d/$ISSUE"
				continue
			elif [ -f $ROOT/.reproduce.d/${ISSUE}.txt ]; then
				echo -1 > $ROOT/.reproduce.d/${ISSUE}.txt
			fi

			"$PIPELINE/$LIBRARIES/Reproduce.sh" clone "$ISSUE" "$REPO" "$ROOT" "$SPEC" "$AUTH" "$COMMIT" "$REVS"

			if [ $? != 0 ]; then
				CODE=$?
				continue
			elif [ -d "$ROOT/.reproduce.d/$ISSUE" ]; then
				FOUND=0
				
				MOD=$(python -c "print(\"$ISSUE\".split(\"_\")[0])") >& /dev/null

				if [ $? != 0 ]; then
					error "not found issue $ISSUE"
				elif [ ! -f $ROOT/.reproduce.d/${ISSUE}.txt ]; then
					if ! "$PIPELINE/$LIBRARIES/Reproduce.sh" prepare "$ISSUE" "$ROOT" $MOD; then
						CODE=$?
						continue
					else
						echo -1 > $ROOT/.reproduce.d/${ISSUE}.txt
					fi
				fi
	
				progress $IDX "Preproducing ->"

				LIB=$(python -c "print(\"$ISSUE\".split(\"_\")[1])") >& /dev/null
				if [ $? != 0 ]; then
					error "not found issue $ISSUE"
				fi
	
				SUITE=$(python -c "print(\"$ISSUE\".split(\"_\")[2].lower())") >& /dev/null
				if [ $? != 0 ]; then
					error "not found issue $ISSUE"
				fi
	
				CASE=$(python -c "print(\"$ISSUE\".split(\"_\")[3])") >& /dev/null
				if [ $? != 0 ]; then
					error "not found issue $ISSUE"
				fi	
	
				if [[ $FOUND -eq 0 ]]; then
					rm -fr "$LOG/$ISSUE"
					touch "$LOG/$ISSUE"
				fi
	
				"$PIPELINE/$LIBRARIES/Reproduce.sh" inject "$ISSUE" "$ROOT" "$CODE"
				if [ $? != 0 ]; then
					error "fait to inject reproducing scripts"
				fi

				"$PIPELINE/$LIBRARIES/Reproduce.sh" reproduce "$ISSUE" "$ROOT" "$LOG/$ISSUE" "$LIB" "$SUITE" "$CASE"
				CODE=$?

				if [ $CODE != 0 ]; then
					"$PIPELINE/$LIBRARIES/Reproduce.sh" verify "$ISSUE" "$ROOT" "$LOG/$ISSUE" "$CODE"
	
					if [ $? != 0 ]; then
						FOUND=1
					else
						echo ' fail' | tr -d '\n'
					fi
				else
					echo ' fail' | tr -d '\n'
				fi

				END=$(date +%s)
	
				if [[ ${#TIMEOUT} -gt 0 ]] && [[ $((END-BEGIN)) -gt $TIMEOUT ]]; then
					exit 0
				else
					IDX=$((IDX+1))
				fi

				echo $IDX > $ROOT/.reproduce.d/${ISSUE}.txt

				if [ $FOUND != 0 ]; then
					if [ $(wc -l "$LOG/$ISSUE" | awk '{ print $1 }') -gt 10000 ]; then
						echo " success. The log looks too big to be showing on console Please check your storage"
					else
						echo " success. Here is the log:"
						echo ""
						cat "$LOG/$ISSUE"
					fi
	
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
			
					$PIPELINE/$LIBRARIES/Reproduce.sh report "$ISSUE" "$ROOT" "$LOG/$ISSUE" $MOD

					if [ ! -e "$ROOT/BUG" ]; then
						touch "$ROOT/BUG"
						break
					fi
				fi
			else
				warning "Fail on preparing the issue $ISSUE"
			fi
		done

		COUNT=0

		if [ ! -d $ROOT/.reproduce.d ]; then
			break
		fi

		for NAME in $(ls -1c $ROOT/.reproduce.d/); do
			if [ -d $ROOT/.reproduce.d/$NAME ]; then
				COUNT=$((COUNT+1))
			fi
		done

		if [[ $COUNT -eq 0 ]] || [ -e $ROOT/BUG ]; then
			break
		fi
	done
fi

COUNT=0

for NAME in $(ls -1c $ROOT/.reproduce.d/); do
	if [ -f $ROOT/.reproduce.d/$NAME ]; then
		COUNT=$((COUNT+1))
	fi
done

if [[ $COUNT -eq 0 ]] || [ -e $ROOT/BUG ]; then
	echo ''
fi

if [ "$CODE" != 0 ]; then
	error "Got problems during reproduce"
elif [ ! -e "$ROOT/BUG" ]; then
	info "It seems the issues have been fixed completely"
else
	exit -1
fi

