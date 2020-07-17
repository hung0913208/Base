#!/bin/bash

SAVE=$IFS
KEEP=1

IFS=$'\n'
IGNORANCEs=($(git log --format=%B -n 1 HEAD | grep " Ignored "))
IFS=$SAVE

if [ -f $HOME/environment.sh ]; then
	source $HOME/environment.sh
fi

for IGNORE in ${IGNORANCEs[@]}; do
	if echo $IGNORE | grep "reproduce.sh\|all"; then
		KEEP=0
	fi
done

if [[ $KEEP -eq 0 ]]; then
	exit 0
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
	elif [ -d $root/../.git ]; then
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

	if mv $(dirname $0)/tasks/reproduce.sh $(dirname $0)/tasks/reproduce-$DATE.sh; then
		$(dirname $0)/tasks/reproduce-$DATE.sh run $@ | eval "$(console)"
		CODE=${PIPESTATUS[0]}

		rm -fr $(dirname $0)/tasks/reproduce-$DATE.sh
		clean $@

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

	for I in $(seq -s ' ' 1 $#); do
		IPLUS=$((I+1))

		case ${!I} in
			--resource)  ENVIRONMENT=${!IPLUS};;
			(--)		;;
			(*)		;;
		esac
	done

	if [[ ${#ENVIRONMENT} -gt 0 ]]; then
		source $ENVIRONMENT
	fi

	if flock -n -x 200; then
		trap "flock --unlock 200" EXIT

		if [ ! -f $(dirname $0)/tasks/reproduce.sh ]; then
			if [[ ${#SERVICES} -gt 0 ]]; then
				for SERVICE in ${SERVICES[@]}; do
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
			elif [ -f $HOME/reproduce-services.list ]; then
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

	if [[ $CODE -eq 0 ]]; then
		echo $(token) > /var/lock/$(whoami)-resignd.lck
	else
		rm -fr $(dirname $0)/tasks/*.sh
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

