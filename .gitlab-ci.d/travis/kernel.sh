#!/bin/bash

BASE=$(realpath $(dirname $0)/../../)
SAVE=$IFS
KEEP=1

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

	if [ -f /var/lock/travis/${CI_JOB_TOKEN} ]; then
		while ! $VERBOSE /var/lock/travis/${CI_JOB_TOKEN}; do
			sleep 1
		done

		rm -fr /var/lock/travis/${CI_JOB_TOKEN}
	else
		$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}
		$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $STOP --token ${TRAVIS} --repo ${REPO}
	fi
}

function run() {
	LABs=''
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x";;
			--labs)		LABs="$2"; shift;;
			--os)		shift;;
			(--) 		shift; break;;
			(*) 		break;;
		esac
		shift
	done

	for IDX in $(seq $BEGIN $END); do
		for JOB in $(python -c "for v in '$LABs'.split(';'): print(v)"); do
			STATUS=$($VERBOSE $BASE/Tools/Utilities/travis.sh status --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO})

			if [ $STATUS = 'passed' ] || [ $STATUS = 'failed' ] || [ $STATUS = 'canceled' ] || [ $STATUS = 'errored' ]; then
				if [ -f /var/lock/travis/${CI_JOB_TOKEN} ]; then
					$VERBOSE $BASE/Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO} --script /var/lock/travis/${CI_JOB_TOKEN}
					CODE=$?
				else
					$VERBOSE $BASE/Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO}
					CODE=$?
				fi

				$VERBOSE $BASE/Tools/Utilities/travis.sh delete --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO}	
				rm -fr /var/lock/travis/${CI_JOB_TOKEN}
				exit $CODE
			fi
		done
	done

	exit -1
}

function probe() {
	VERBOSE="bash"

	mkdir -p /var/lock/travis

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x";;
			(--)		shift; break;;
			(*)		break;;
		esac
		shift
	done

	if $VERBOSE $BASE/Tools/Utilities/travis.sh env exist --name "$START" --token ${TRAVIS} --repo ${REPO}; then
		exit -1
	fi
}

function plan() {
	VERBOSE="bash"

	while [ $# -gt 0 ]; do
		case $1 in
			--verbose)	VERBOSE="bash -x";;
			(--)		shift; break;;
			(*)		break;;
		esac
		shift
	done

	if ! $VERBOSE $BASE/Tools/Utilities/travis.sh env add --name "$START" --value "$HOOK" --token ${TRAVIS} --repo ${REPO}; then
		exit -1
	fi

	if [[ $(ls -1l /var/lock/travis/ | wc -l) -gt 4 ]]; then
		if ! $VERBOSE $BASE/Tools/Utilities/travis.sh env add --name "$STOP" --value "$NOTIFY" --token ${TRAVIS} --repo ${REPO}; then
			$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}
			exit -1
		fi
	else
		cat > /var/lock/travis/${CI_JOB_TOKEN} << EOF
#!/bin/bash

$VERBOSE $BASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}
EOF
		chmod +x /var/lock/travis/${CI_JOB_TOKEN}
	fi
}

CMD=$1
shift

case $CMD in
	run) 		run $@;;
	plan) 		plan $@;;
	probe) 		probe $@;;
	clean) 		clean $@;;
	(*)		exit -1;;
esac
