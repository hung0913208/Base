#!/bin/bash

SAVE=$IFS
KEEP=1

IFS=$'\n'
IGNORANCEs=($(git log --format=%B -n 1 HEAD | grep " Ignored "))
IFS=$SAVE

for IGNORE in ${IGNORANCEs[@]}; do
	if echo $IGNORE | grep "kernel.sh\|all"; then
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

function token() {	
	echo "${CI_JOB_TOKEN}"
}

function console() {
	if [ -f $HOME/console.sh ]; then
		if [ -d $ROOT/../.git ]; then
			if ! $HOME/console.sh --register "$(realpath $ROOT/../)" --uid $(token); then
				echo "tee -a $ROOT/../console.log"
			fi
		else
			if ! $HOME/console.sh --register "$ROOT" --uid $(token); then
				echo "tee -a $ROOT/console.log"
			fi
		fi
	elif [ -d $ROOT/../.git ]; then
		echo "tee -a $ROOT/../console.log"
	else
		echo "tee -a $ROOT/console.log"
	fi
}

function clean() {
	if [ -f $HOME/console.sh ]; then
		if [ -d $ROOT/../.git ]; then
			$HOME/console.sh --unregister "$(realpath $ROOT/../)" --uid $(token)
		else
			$HOME/console.sh --unregister "$ROOT" --uid $(token)
		fi
	else
		if [ -d $ROOT/../.git ]; then
			rm -fr $ROOT/../console.log
		else
			rm -fr $ROOT/console.log
		fi
	fi

	if [ -f $(dirname $0)/tasks/build.sh ]; then
		$(dirname $0)/tasks/build.sh clean $@
	fi
}

function run() {
	DATE=$(date +%s)

	if mv $(dirname $0)/tasks/kernel.sh $(dirname $0)/tasks/kernel-$DATE.sh; then
		$(dirname $0)/tasks/kernel-$DATE.sh run $@ | eval "$(console)"
		CODE=${PIPESTATUS[0]}

		rm -fr $(dirname $0)/tasks/kernel-$DATE.sh
		clean $@

		if git log --format=%B -n 1 HEAD | grep " Expect failed" >& /dev/null; then
			if [[ $CODE -ne 0 ]]; then
				exit 0
			else
				exit -1
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

		if [ ! -f $(dirname $0)/tasks/kernel.sh ]; then
			if [ -f $HOME/kernel-services.list ]; then
				for SERVICE in $(cat $HOME/build-services.list); do
					SERVICE=$ROOT/.gitlab-ci.d/$SERVICE

					if [ -d $SERVICE ]; then
						if [ ! -f $SERVICE/kernel.sh ]; then
							continue
						elif $SERVICE/kernel.sh probe $@; then
							if ln -s $SERVICE/kernel.sh $(dirname $0)/tasks/kernel.sh; then
								CODE=0
							fi
						fi
					fi
				done
			else
				for SERVICE in $ROOT/.gitlab-ci.d/*; do
					if [ -d $SERVICE ]; then
						if [ ! -f $SERVICE/kernel.sh ]; then
							continue
						elif $SERVICE/kernel.sh probe $@; then
							if ln -s $SERVICE/kernel.sh $(dirname $0)/tasks/kernel.sh; then
								CODE=0
							fi
						fi
					fi
				done
			fi
		fi
	fi

	if [[ $CODE -eq 0 ]]; then
		echo $CI_JOB_TOKEN > /var/lock/$(whoami)-resignd.lck
	else
		rm -fr $(dirname $0)/tasks/*.sh
	fi

	exit $CODE
}

function plan() {
	if [ ! -f /var/lock/$(whoami)-resignd.lck ]; then
		exit -1
	elif [ $(cat /var/lock/$(whoami)-resignd.lck) != $(token) ]; then
		exit -1
	else
		CODE=0

		if ! $(dirname $0)/tasks/kernel.sh plan $@; then
			rm -fr $(dirname $0)/tasks/kernel.sh
			CODE=-1
		fi

		rm -fr /var/lock/$(whoami)-resignd.lck
		exit $CODE
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

