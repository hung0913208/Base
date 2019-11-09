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

ROOT=$(realpath $(dirname $0)/../)

function clean() {
	if [ -f $(dirname $0)/tasks/build.sh ]; then
		$(dirname $0)/tasks/build.sh $@
	fi
}

function run() {
	DATE=$(date +%s)

	if mv $(dirname $0)/tasks/build.sh $(dirname $0)/tasks/build-$DATE.sh; then
		$(dirname $0)/tasks/build-$DATE.sh run $@
		CODE=$?

		rm -fr $(dirname $0)/tasks/build-$DATE.sh

		if git log --format=%B -n 1 HEAD | grep " Expect failed" >& /dev/null; then
			if [[ $CODE -ne 0 ]]; then
				exit 0
			else
				exit -1
			fi
		else
			exit $CODE
		fi
	fi
}

function probe() {
	CODE=-1
	exec 200>/var/lock/lockd.lck

	if flock -n -x 200; then
		trap "flock --unlock 200" EXIT

		if [ ! -f $(dirname $0)/tasks/build.sh ]; then
			for SERVICE in $ROOT/.gitlab-ci.d/*; do
				if [ -d $SERVICE ]; then
					if [ ! -f $SERVICE/build.sh ]; then
						continue
					elif $SERVICE/build.sh probe $@; then
						if ln -s $SERVICE/build.sh $(dirname $0)/tasks/build.sh; then
							CODE=0
						fi
					fi
				fi
			done
		fi
	fi

	if [[ $CODE -eq 0 ]]; then
		echo $CI_JOB_TOKEN > /var/lock/resignd.lck
	fi

	exit $CODE
}

function plan() {
	if [ ! -f /var/lock/resignd.lck ]; then
		exit -1
	elif [ $(cat /var/lock/resignd.lck) != ${CI_JOB_TOKEN} ]; then
		exit -1
	else
		CODE=0

		if ! $(dirname $0)/tasks/build.sh plan $@; then
			rm -fr $(dirname $0)/tasks/build.sh
			CODE=-1
		fi

		rm -fr /var/lock/resignd.lck
		exit $CODE
	fi
}

CMD=$1
shift

mkdir -p $(dirname $0)/tasks
case $CMD in
	run) 		run $@;;
	plan) 		plan $@;;
	probe) 		probe $@;;
	clean) 		clean $@;;
	(*)		exit -1;;
esac

