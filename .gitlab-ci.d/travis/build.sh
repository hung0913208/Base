#!/bin/bash

for JOB in $(python -c "for v in '$1'.split(';'): print(v)"); do
	RUN=0

	for IDX in `seq ${BEGIN} ${END}`; do
		STATUS=$(./Tools/Utilities/travis.sh status --job ${JOB} --patch ${IDX} --token ${TOKEN} --repo ${REPO})

		if [ $STATUS != 'started' ]; then
			RUN=1

			if ! ./Tools/Utilities/travis.sh restart --job ${JOB} --patch ${IDX} --token ${TOKEN} --repo ${REPO}; then
				exit -1
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
