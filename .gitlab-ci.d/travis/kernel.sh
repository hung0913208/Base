#!/bin/bash

BASE=$(realpath $(dirname $0)/../../)
SAVE=$IFS
KEEP=1

if [ -f $HOME/environment.sh ]; then
	source $HOME/environment.sh
fi

IFS=$'\n'
IGNORANCEs=($(git log --format=%B -n 1 HEAD | grep " Ignored "))
IFS=$SAVE

for IGNORE in ${IGNORANCEs[@]}; do
	if echo $IGNORE | grep "kenrel.sh\|all"; then
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

START="HOOK"
STOP="NOTIFY"
HOOK="\\\"export JOB='build'; sudo apt install qemu; echo '$REPOSITORY $BRANCH' >> ./repo.list\\\""
NOTIFY="\\\"../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}; ../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $STOP --token ${TRAVIS} --repo ${REPO}\\\""

function lock() {
	while [ 1 ]; do
		if flock -n -x 200; then
			trap "flock --unlock 200" EXIT

			echo $(($(date +%s))) > /var/lock/$(whoami)/travis-timelock.lck
			break
		fi
	done
}

function unlock() {
	if flock -n -x 200; then
		trap "flock --unlock 200" EXIT

		if [ -f /var/lock/$(whoami)/travis-timelock.lck ]; then
			if [[ $(date +%s) -lt $(cat /var/lock/$(whoami)/travis-timelock.lck) ]]; then
				exit -1
			fi
		fi
	else
		exit -1
	fi
}

function check() {
	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	set -x;;
			(--)		shift; break;;
			(*)		break;;
		esac
		shift
	done

	if ! unlock; then
		exit -1
	fi
}
function clean() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--environment)  ENVIRONMENT=$2; shift;;
			--verbose)	VERBOSE="bash -x"; set -x;;
			(--)		shift; break;;
			(*)		break;;
		esac
		shift
	done

	if [ -f /var/lock/$(whoami)/travis/${CI_JOB_TOKEN} ]; then
		COUNT=0
		while ! $VERBOSE /var/lock/$(whoami)/travis/${CI_JOB_TOKEN}; do
			COUNT=$((COUNT+1))

			if [[ $COUNT -lt 60 ]]; then
				sleep 1
			else
				break
			fi
		done

		rm -fr /var/lock/$(whoami)/travis/${CI_JOB_TOKEN}
	else
		$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}
		$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $STOP --token ${TRAVIS} --repo ${REPO}

		if [[ ${#ENVIRONMENT} -gt 0 ]]; then
			SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
			KEY=${SPLITED[0]}

			cat $ENVIRONMENT | while read DEFINE; do
				$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $KEY --token ${TRAVIS} --repo ${REPO}
			done
		fi
	fi
}

function run() {
	LABs=''
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x"; set -x;;
			--labs)		LABs="$2"; shift;;
			--os)		shift;;
			(*) 		PARAMS="$PARAMS $1";;
		esac
		shift
	done

	for IDX in $(seq $BEGIN $END); do
		for JOB in $(python -c "for v in '$LABs'.split(';'): print(v)"); do
			STATUS=$($VERBOSE $BASE/Tools/Utilities/travis.sh status --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO})

			if [ $STATUS = 'passed' ] || [ $STATUS = 'failed' ] || [ $STATUS = 'canceled' ] || [ $STATUS = 'errored' ]; then
				if [ -f /var/lock/$(whoami)/travis/${CI_JOB_TOKEN} ]; then
					$VERBOSE $BASE/Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO} --script /var/lock/$(whoami)/travis/${CI_JOB_TOKEN}
					CODE=$?
				else
					$VERBOSE $BASE/Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO}
					CODE=$?
				
					if [[ $CODE -ne 0 ]]; then
						if [[ ${#VERBOSE} -gt 0 ]]; then
							clean --verbose $PARAMS $@
						else
							clean $PARAMS $@
						fi
					fi
				fi

				$VERBOSE $BASE/Tools/Utilities/travis.sh delete --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO}	
				rm -fr /var/lock/$(whoami)/travis/${CI_JOB_TOKEN}
				exit $CODE
			fi
		done
	done

	if [[ ${#VERBOSE} -gt 0 ]]; then
		clean --verbose
	else
		clean
	fi
	exit -1
}

function probe() {
	VERBOSE="bash"

	mkdir -p /var/lock/$(whoami)/travis

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x"; set -x;;
			(--)		shift; break;;
			(*)		shift;;
		esac
		shift
	done

	if ! unlock; then
		exit -1
	fi

	if $VERBOSE $BASE/Tools/Utilities/travis.sh env exist --name "$START" --token ${TRAVIS} --repo ${REPO}; then
		if [[ $(ls -1l /var/lock/$(whoami)/travis/ | wc -l) -lt 4 ]]; then
			if $VERBOSE $BASE/Tools/Utilities/travis.sh env exist --name "$STOP" --token ${TRAVIS} --repo ${REPO}; then
				if $VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $STOP --token ${TRAVIS} --repo ${REPO}; then
					exit -1
				elif $VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}; then
					exit -1
				else
					exit 0
				fi
			fi
		fi

		exit -1
	fi
}

function plan() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x"; set -x;;
			--environment)  ENVIRONMENT=$2; shift;;
			(--)		shift; break;;
			(*)		shift;;
		esac
		shift
	done

	if ! $VERBOSE $BASE/Tools/Utilities/travis.sh env add --name "$START" --value "$HOOK" --token ${TRAVIS} --repo ${REPO}; then
		lock
		exit -1
	fi

	if [[ $(ls -1l /var/lock/$(whoami)/travis/ | wc -l) -gt 4 ]]; then
		touch /var/lock/$(whoami)/travis/${CI_JOB_TOKEN}

		if ! $VERBOSE $BASE/Tools/Utilities/travis.sh env add --name "$STOP" --value "$NOTIFY" --token ${TRAVIS} --repo ${REPO}; then
			$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}

			if [[ ${#ENVIRONMENT} -gt 0 ]]; then
				SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
				KEY=${SPLITED[0]}

				cat $ENVIRONMENT | while read DEFINE; do
					$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $KEY --token ${TRAVIS} --repo ${REPO}
				done
			fi

			lock
			exit -1
		fi
	else
		cat > /var/lock/$(whoami)/travis/${CI_JOB_TOKEN} << EOF
#!/bin/bash

$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}
EOF
		chmod +x /var/lock/$(whoami)/travis/${CI_JOB_TOKEN}
	fi

	if [[ ${#ENVIRONMENT} -gt 0 ]]; then
		cat $ENVIRONMENT | while read DEFINE; do
			SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
			KEY=${SPLITED[0]}
			VAL=${SPLITED[1]}

			if ! $VERBOSE $BASE/Tools/Utilities/travis.sh env add --name "$KEY" --value "$VAL" --token ${TRAVIS} --repo ${REPO}; then
				lock
				exit -1
			else
				cat >> /var/lock/$(whoami)/travis/${CI_JOB_TOKEN} << EOF
$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $KEY --token ${TRAVIS} --repo ${REPO}
EOF
				chmod +x /var/lock/$(whoami)/travis/${CI_JOB_TOKEN}
			fi
		done
	fi
}

CMD=$1
shift

case $CMD in
	run) 		run $@;;
	plan) 		plan $@;;
	probe) 		probe $@;;
	clean) 		clean $@;;
	check) 		check $@;;
	(*)		exit -1;;
esac
