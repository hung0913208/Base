#!/bin/bash

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

for JOB in $(python -c "for v in '$1'.split(';'): print(v)"); do
	RUN=0

	for IDX in `seq ${BEGIN} ${END}`; do
		STATUS=$(./Tools/Utilities/travis.sh status --job ${JOB} --patch ${IDX} --token ${TOKEN} --repo ${REPO})
		CODE=0

		if [ $STATUS = 'passed' ] || [ $STATUS = 'failed' ] || [ $STATUS = 'canceled' ] || [ $STATUS = 'errored' ]; then
			RUN=1
			START="HOOK${JOB}_gitlab_${CI_JOB_ID}"
			STOP="NOTIFY${JOB}_gitlab_${CI_JOB_ID}"
			HOOK="\\\"if [ \\\\\$TRAVIS_JOB_NUMBER = '$IDX.$JOB' ]; then export JOB='build'; sudo apt install qemu; echo '$REPOSITORY $BRANCH' >> ./repo.list; ADOPTED=1; fi\\\""
			NOTIFY="\\\"if [ \\\\\$TRAVIS_JOB_NUMBER = '$IDX.$JOB' ]; then ../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $START --token ${TOKEN} --repo ${REPO}; ../\\\\\$LIBBASE/Tools/Utilities/travis.sh env del --name $STOP --token ${TOKEN} --repo ${REPO}; fi\\\""

			if [ -d /tmp/jobs/$(date +%j) ]; then			
				cat > /tmp/jobs/$(date +%j)/${CI_CONCURRENT_ID}_${CI_CONCURRENT_PROJECT_ID} << EOF
./Tools/Utilities/travis.sh env del --name "$START" --token ${TOKEN} --repo ${REPO}
./Tools/Utilities/travis.sh env del --name "$STOP" --token ${TOKEN} --repo ${REPO}
EOF
			fi

			./Tools/Utilities/travis.sh env add --name "$START" --value "$HOOK" --token ${TOKEN} --repo ${REPO}
			./Tools/Utilities/travis.sh env add --name "$STOP" --value "$NOTIFY" --token ${TOKEN} --repo ${REPO}
			if ! ./Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TOKEN} --repo ${REPO}; then
				CODE=-1
			fi

			./Tools/Utilities/travis.sh env del --name "$START" --token ${TOKEN} --repo ${REPO}
			./Tools/Utilities/travis.sh env del --name "$STOP" --token ${TOKEN} --repo ${REPO}
			if [ $CODE != 0 ]; then
				exit $CODE
			else
				break
			fi
		fi
	done

	if [[ $RUN -eq 0 ]]; then
		echo "[  ERROR  ]: Node so busy to perform your task, please try again later or expand the range"
		exit -2
	fi
done
