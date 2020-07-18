#!/bin/bash

SAVE=$IFS
KEEP=1

if [ -f $HOME/environment.sh ]; then
	source $HOME/environment.sh
fi

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

BASE=$(realpath $(dirname $0)/../../)
START="HOOK"
STOP="NOTIFY"
HOOK="export JOB=build; apt install -y qemu; echo \\\"$REPOSITORY $BRANCH\\\" >> ./repo.list"
NOTIFY="/pipeline/source/Base/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}; /pipeline/source/Base/Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}; apt install -y qemu"

function lock() {
	mkdir -p /var/lock/$(whoami)
	exec 200>/var/lock/$(whoami)/wecker-timelock.lck

	while [ 1 ]; do
		if flock -n -x 200; then
			trap "flock --unlock 200" EXIT

			echo $(($(date +%s))) > /var/lock/$(whoami)/wercker-timelock.lck
			break
		fi
	done
}

function unlock() {
	mkdir -p /var/lock/$(whoami)
	exec 200>/var/lock/$(whoami)/wecker-timelock.lck

	if flock -n -x 200; then
		trap "flock --unlock 200" EXIT

		if [ -f /var/lock/$(whoami)/wercker-timelock.lck ]; then
			if [[ $(date +%s) -lt $(cat /var/lock/$(whoami)/wercker-timelock.lck) ]]; then
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
			(*)		shift;;
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
			(*)		shift;;
		esac
		shift
	done

	if [ -f /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN} ]; then
		COUNT=0
		while ! $VERBOSE /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN}; do
			COUNT=$((COUNT+1))

			if [[ $COUNT -lt 60 ]]; then
				sleep 1
			else
				break
			fi
		done

		rm -fr /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN}
	else
		$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}
		$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}

		if [[ ${#ENVIRONMENT} -gt 0 ]]; then
			cat $ENVIRONMENT | while read DEFINE; do
				SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
				KEY=${SPLITED[0]}

				$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $KEY --token ${WERCKER} --repo ${REPO}
			done
		fi
	fi
}

function run() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x"; set -x;;
			(*) 		PARAMS="$PARAMS $1";;
		esac
		shift
	done

	if [ -f /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN} ]; then
		$VERBOSE $BASE/Tools/Utilities/wercker.sh restart --token ${WERCKER} --repo ${REPO} --script /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN}
		CODE=$?
	else
		$VERBOSE $BASE/Tools/Utilities/wercker.sh restart --token ${WERCKER} --repo ${REPO}
		CODE=$?

		if [[ $CODE -ne 0 ]]; then
			if [[ ${#VERBOSE} -eq 0 ]]; then
				clean $PARAMS
			else
				clean --verbose $PARAMS
			fi
		fi
	fi

	rm -fr /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN}
	exit $CODE
}

function probe() {
	VERBOSE="bash"

	mkdir -p /var/lock/$(whoami)/wercker

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x"; set -x;;
			--os)		OS="$2"; shift;;
			--labs)		shift;;
			(--) 		shift; break;;
			(*) 		shift;;
		esac
		shift
	done

	if ! unlock; then
		exit -1
	fi

	if [ $OS = 'linux' ]; then
		if ! $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
			if [[ $(ls -1l /var/lock/$(whoami)/wercker/ | wc -l) -lt 2 ]]; then
				if $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$STOP" --value "$NOTIFY" --token ${WERCKER} --repo ${REPO}; then
					while ! $VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}; do
						sleep 1
					done

					if $VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}; then
						exit -1
					else
						if ! $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
							exit -1
						fi

						exit 0
					fi
				elif $VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}; then
					$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}

					if $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
						exit 0
					fi
				fi

				exit -1
			fi
		fi
	else
		exit -1
	fi
}

function plan() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x"; set -x;;
			--environment)  ENVIRONMENT=$2; shift; break;;
			(--)		shift; break;;
			(*)		shift;;
		esac
		shift
	done

	if [[ $(ls -1l /var/lock/$(whoami)/wercker/ | wc -l) -gt 2 ]]; then
		if ! $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$STOP" --value "$NOTIFY" --token ${WERCKER} --repo ${REPO}; then
			$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}

			if [[ ${#ENVIRONMENT} -gt 0 ]]; then
				cat $ENVIRONMENT | while read DEFINE; do
					SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
					KEY=${SPLITED[0]}

					$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $KEY --token ${WERCKER} --repo ${REPO}
				done
			fi

			lock
			exit -1
		fi
	else
		cat > /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN} << EOF
#!/bin/bash

$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}
EOF
		chmod +x /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN}
	fi

	if [[ ${#ENVIRONMENT} -gt 0 ]]; then
		cat $ENVIRONMENT | while read DEFINE; do
			SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
			KEY=${SPLITED[0]}
			VAL=${SPLITED[1]}

			if ! $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$KEY" --value "$VAL" --token ${WERCKER} --repo ${REPO}; then
				lock
				exit -1
			else
				cat >> /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN} << EOF
$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $KEY --token ${WERCKER} --repo ${REPO}
EOF
		chmod +x /var/lock/$(whoami)/wercker/${CI_JOB_TOKEN}
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

