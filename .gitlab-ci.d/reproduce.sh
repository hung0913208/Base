#!/bin/bash

SAVE=$IFS
KEEP=1

IFS=$'\n'
IGNORANCEs=($(git log --format=%B -n 1 HEAD | grep " Ignored "))
IFS=$SAVE

for IGNORE in ${IGNORANCEs[@]}; do
	if echo $IGNORE | grep "reproduce.sh\|all"; then
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

	if mv $(dirname $0)/tasks/reproduce.sh $(dirname $0)/tasks/reproduce-$DATE.sh; then
		$(dirname $0)/tasks/reproduce-$DATE.sh run $@
		CODE=$?

		rm -fr $(dirname $0)/tasks/reproduce-$DATE.sh

		if git log --format=%B -n 1 HEAD | grep " Expect failed" >& /dev/null; then
			if [[ $CODE -ne 0 ]]; then
				exit 0
			else
				IFS=$'\n'
				ISSUEs=($(git log --format=%B -n 1 HEAD | grep " Ignored "))
				IFS=$SAVE

				if [[ ${#ISSUEs} -gt 0 ]]; then
					exit -1
				else
					exit 0
				fi
			fi
		else
			exit $CODE
		fi
	else
		clean $@
	fi
}

function probe() {
	CODE=-1
	exec 200>/var/lock/$(whoami)-lockd.lck

	if flock -n -x 200; then
		trap "flock --unlock 200" EXIT

		if [ ! -f $(dirname $0)/tasks/reproduce.sh ]; then
			if [ -f $HOME/reproduce-services.list ]; then
				for SERVICE in $(cat $HOME/reproduce-services.list); do
					SERVICE=$ROOT/.gitlab-ci.d/$SERVICE

					if [ -d $SERVICE ]; then
						if [ ! -f $SERVICE/reproduce.sh ]; then
							continue
						elif $SERVICE/reproduce.sh probe $@; then
							if ln -s $SERVICE/reproduce.sh $(dirname $0)/tasks/reproduce.sh; then
								CODE=0
							fi
						fi
					fi
				done
			else
				for SERVICE in $ROOT/.gitlab-ci.d/*; do
					if [ -d $SERVICE ]; then
						if [ ! -f $SERVICE/reproduce.sh ]; then
							continue
						elif $SERVICE/reproduce.sh probe $@; then
							if ln -s $SERVICE/reproduce.sh $(dirname $0)/tasks/reproduce.sh; then
								CODE=0
							fi
						fi
					fi
				done
			fi
		fi
	fi
	exit $CODE
}

function plan() {
	if ! $(dirname $0)/tasks/reproduce.sh plan $@; then
		rm -fr $(dirname $0)/tasks/reproduce.sh
		exit -1
	fi
}

function check() {
	CODE=-1

	if flock -n -x 200; then
		trap "flock --unlock 200" EXIT

		if [ ! -f $(dirname $0)/tasks/build.sh ]; then
			for SERVICE in $ROOT/.gitlab-ci.d/*; do
				if [ -d $SERVICE ]; then
					if [ ! -f $SERVICE/build.sh ]; then
						continue
					elif $SERVICE/build.sh check $@; then
						CODE=0
					fi
				fi
			done
		fi

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
	check) 		check $@;;
	(*)		exit -1;;
esac

