#!/bin/bash

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

if [[ ${#REPOSITORY} -eq 0 ]] || [[ ${#BRANCH} -eq 0 ]]; then
	REPOSITORY=${CI_REPOSITORY_URL}
	BRANCH=${CI_COMMIT_REF_NAME}
fi

if [[ ${#USERNAME} -gt 0 ]] && [[ ${#PASSWORD} -gt 0 ]]; then
	PROTOCOL=$(python -c "print('${REPOSITORY}'.split('://')[0])")
	REPOSITORY="${PROTOCOL}://$USERNAME:$PASSWORD@$(python -c "print('${REPOSITORY}'.split('://')[1].split('@')[1])")"
fi

ROOT=$(realpath $(dirname $0)/../../)
START="HOOK"
STOP="NOTIFY"
HOOK="export JOB=build; echo \\\"$REPOSITORY $BRANCH\\\" >> ./repo.list"
NOTIFY="/pipeline/source/Base/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}; /pipeline/source/Base/Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}"

function run() {
	$ROOT/Tools/Utilities/wercker.sh restart --token ${WERCKER} --repo ${REPO}
	exit $?
}

function probe() {
	$ROOT/Tools/Utilities/wercker.sh env add --name "$STOP" --value "$NOTIFY" --token ${WERCKER} --repo ${REPO}
	exit $?
}

function plan() {
	if ! $ROOT/Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
		exit -1
	fi
}

CMD=$1
shift

case $CMD in
	run) 		run $@;;
	plan) 		plan $@;;
	probe) 		probe $@;;
	(*)		exit -1;;
esac

