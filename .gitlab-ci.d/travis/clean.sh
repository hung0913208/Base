#!/bin/bash

POS=$1
JOB=$2
IDX=$3

if [ $POS = 'master' ]; then
	./Tools/Utilities/travis.sh env list --token ${TOKEN} --repo ${REPO} --script "/usr/bin/bash $0 slaver $2 $3"
elif [ $POS = 'all' ]; then
	for CRON in /tmp/jobs/*; do
		if [ ! -d $CRON ]; then
			continue
		fi

		for SCRIPT in $CRON/*; do
			bash $SCRIPT
		done
	done
else
	NAME=$4
	VALUE=$5

	if [[ $NAME =~ "HOOK$JOB"* ]]; then
		if ! echo $VALUE | grep "$JOB.$IDX" >& /dev/null; then
			./Tools/Utilities/travis.sh env del --name $NAME --token ${TOKEN} --repo ${REPO}
		fi
	fi
fi
