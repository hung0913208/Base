#!/bin/bash

for JOB in $(python -c "for v in '$1'.split(';'): print(v)"); do
	RUN=0

	for IDX in `seq ${BEGIN} ${END}`; do
		STATUS=$(./Tools/Utilities/travis.sh status --job ${JOB} --patch ${IDX} --token ${TOKEN} --repo ${REPO})
		CODE=0

		if [ $STATUS = 'passed' ] || [ $STATUS = 'failed' ] || [ $STATUS = 'canceled' ] || [ $STATUS = 'errored' ]; then
			RUN=1
			HOOK="HOOK${JOB}_gitlab_${CI_JOB_ID}"
			VALUE="\\\"if [ \\\\\$TRAVIS_JOB_NUMBER = '$IDX.$JOB' ]; then export JOB='build'; echo '${CI_REPOSITORY_URL} ${CI_COMMIT_REF_NAME}' >> ./repo.list; ADOPTED=1; fi\\\""

			./Tools/Utilities/travis.sh env add --name "$HOOK" --value "$VALUE" --token ${TOKEN} --repo ${REPO}			
			if ! ./Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TOKEN} --repo ${REPO}; then
				CODE=-1
			fi

			./Tools/Utilities/travis.sh env del --name "$HOOK" --token ${TOKEN} --repo ${REPO}
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
