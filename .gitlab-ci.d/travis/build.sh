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

for JOB in $(python -c "for v in '$1'.split(';'): print(v)"); do
	RUN=0

	for IDX in `seq ${BEGIN} ${END}`; do
		STATUS=$($BASE/Tools/Utilities/travis.sh status --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO})

		if [ $STATUS = 'passed' ] || [ $STATUS = 'failed' ] || [ $STATUS = 'canceled' ] || [ $STATUS = 'errored' ]; then
			START="HOOK${JOB}_gitlab_${CI_JOB_ID}"
			STOP="NOTIFY${JOB}_gitlab_${CI_JOB_ID}"
			HOOK="\\\"if [ \\\\\$TRAVIS_JOB_NUMBER = '$IDX.$JOB' ]; then export JOB='build'; echo '$REPOSITORY $BRANCH' >> ./repo.list; ADOPTED=1; fi\\\""
			NOTIFY="\\\"if [ \\\\\$TRAVIS_JOB_NUMBER = '$IDX.$JOB' ]; then ../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $START --token ${TRAVIS} --repo ${REPO}; ../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $STOP --token ${TRAVIS} --repo ${REPO}; fi\\\""

			if ! $(dirname $0)/clean.sh master $IDX $JOB; then
				continue
			elif [ -d /tmp/jobs/$(date +%j) ]; then			
				cat > /tmp/jobs/$(date +%j)/${CI_CONCURRENT_ID}_${CI_CONCURRENT_PROJECT_ID} << EOF
./Tools/Utilities/travis.sh env del --name "$START" --token ${TRAVIS} --repo ${REPO}
./Tools/Utilities/travis.sh env del --name "$STOP" --token ${TRAVIS} --repo ${REPO}
EOF
			fi

			$BASE/Tools/Utilities/travis.sh env add --name "$START" --value "$HOOK" --token ${TRAVIS} --repo ${REPO}
			$BASE/Tools/Utilities/travis.sh env add --name "$STOP" --value "$NOTIFY" --token ${TRAVIS} --repo ${REPO}

			RUN=1
			if ! $BASE/Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TRAVIS} --repo ${REPO}; then
				CODE=-1
			fi

			break
		fi
	done

	if [[ $RUN -eq 0 ]]; then
		echo "[  ERROR  ]: Node so busy to perform your task, please try again later or expand the range"
		exit -2
	fi
done

exit $CODE
