#!/bin/bash

for JOB in $(python -c "for v in '$1'.split(';'): print(v)"); do
	if ! ./Tools/Utilities/travis.sh restart --job ${JOB} --patch ${PATCH} --token ${TOKEN} --repo ${REPO}; then
		exit -1
	fi
done
