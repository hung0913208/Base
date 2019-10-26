#!/bin/bash

BASE=$(dirname $0)/../../
SAVE=$IFS
KEEP=1

IFS=$'\n'
IGNORANCEs=($(git log --format=%B -n 1 HEAD | grep " Ignored "))
IFS=$SAVE

for IGNORE in ${IGNORANCEs[@]}; do
	if echo $IGNORE | grep "build.sh\|all"; then
		KEEP=0
	fi
done

if [[ $KEEP -eq 0 ]]; then
	exit 0
fi

if [[ ${#REPOSITORY} -gt 0 ]] && [[ ${#BRANCH} -gt 0 ]]; then
	if [[ ${#USERNAME} -gt 0 ]] && [[ ${#PASSWORD} -gt 0 ]]; then
		PROTOCOL=$(python -c "print('${REPOSITORY}'.split('://')[0])")
		REPOSITORY=$(python -c "print('${REPOSITORY}'.split('://')[1])")
		REPOSITORY="${PROTOCOL}://$USERNAME:$PASSWORD@$REPOSITORY"
	fi
else
	REPOSITORY=${CI_REPOSITORY_URL}
	BRANCH=${CI_COMMIT_REF_NAME}
fi

CODE=0
START="HOOK"
STOP="NOTIFY"
HOOK="\\\"export JOB='build'; echo '$REPOSITORY $BRANCH' >> ./repo.list\\\""
NOTIFY="\\\"../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $STOP --token ${TRAVIS} --repo ${REPO};../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}\\\""

function run() {
	LABs=''

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	set -x;;
			--labs)		LABs="$2"; shift;;
			--os)		shift;;
			(--) 		shift; break;;
			(*) 		break;;
		esac
		shift
	done

	for IDX in $(seq $BEGIN $END); do
		for JOB in $(python -c "for v in '$LABs'.split(';'): print(v)"); do
			STATUS=$($BASE/Tools/Utilities/travis.sh status --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO})

			if [ $STATUS = 'passed' ] || [ $STATUS = 'failed' ] || [ $STATUS = 'canceled' ] || [ $STATUS = 'errored' ]; then
				$BASE/Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO}
				exit $?
			fi
		done
	done

	exit -1
}

function probe() {
	mkdir -p /var/lock/travis

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	set -x;;
			(--)		shift; break;;
			(*)		break;;
		esac
		shift
	done

	if $BASE/Tools/Utilities/travis.sh env exist --name "$START" --token ${TRAVIS} --repo ${REPO}; then
		exit -1
	fi
}

function plan() {
	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	set -x;;
			(--)		shift; break;;
			(*)		break;;
		esac
		shift
	done

	$BASE/Tools/Utilities/travis.sh env add --name "$START" --value "$HOOK" --token ${TRAVIS} --repo ${REPO}
	$BASE/Tools/Utilities/travis.sh env add --name "$STOP" --value "$NOTIFY" --token ${TRAVIS} --repo ${REPO}
}

CMD=$1
shift

case $CMD in
	run) 		run $@;;
	plan) 		plan $@;;
	probe) 		probe $@;;
	(*)		exit -1;;
esac
