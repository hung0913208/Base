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

BASE=$(realpath $(dirname $0)/../../)
START="HOOK"
STOP="NOTIFY"
HOOK="export JOB=build; echo \\\"$REPOSITORY $BRANCH\\\" >> ./repo.list"
NOTIFY="/pipeline/source/Base/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}; /pipeline/source/Base/Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}; apt install -y qemu"

function clean() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x";;
			(--)		shift; break;;
			(*)		break;;
		esac
		shift
	done

	if [ -f /var/lock/wercker/${CI_JOB_TOKEN} ]; then
		while ! $VERBOSE /var/lock/wercker/${CI_JOB_TOKEN}; do
			sleep 1
		done

		rm -fr /var/lock/wercker/${CI_JOB_TOKEN}
	else
		$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}
		$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}
	fi
}

function run() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x";;
			(--) 		shift; break;;
			(*) 		break;;
		esac
		shift
	done

	if [ -f /var/lock/wercker/${CI_JOB_TOKEN} ]; then
		$VERBOSE $BASE/Tools/Utilities/wercker.sh restart --token ${WERCKER} --repo ${REPO} --script /var/lock/wercker/${CI_JOB_TOKEN}
		CODE=$?
	else
		$VERBOSE $BASE/Tools/Utilities/wercker.sh restart --token ${WERCKER} --repo ${REPO}
		CODE=$?
	fi

	rm -fr /var/lock/wercker/${CI_JOB_TOKEN}
	exit $CODE
}

function probe() {
	mkdir -p /var/lock/wercker

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	set -x;;
			--os)		OS="$2"; shift;;
			--labs)		shift;;
			(--) 		shift; break;;
			(*) 		break;;
		esac
		shift
	done

	if [ $OS = 'linux' ]; then
		if ! $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
			exit -1
		fi
	else
		exit -1
	fi
}

function plan() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x";;
			(--) 		shift; break;;
			(*) 		break;;
		esac
		shift
	done

	if [[ $(ls -1l /var/lock/wercker/ | wc -l) -gt 2 ]]; then
		if ! $VERBOSE $BASE/Tools/Utilities/wercker.sh env add --name "$STOP" --value "$NOTIFY" --token ${WERCKER} --repo ${REPO}; then
			$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}
			exit -1
		fi
	else
		cat > /var/lock/wercker/${CI_JOB_TOKEN} << EOF
#!/bin/bash

$VERBOSE $BASE/Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}
EOF
		chmod +x /var/lock/wercker/${CI_JOB_TOKEN}
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

