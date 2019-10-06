#!/bin/bash

if [[ ${#REPOSITORY} -eq 0 ]] || [[ ${#BRANCH} -eq 0 ]]; then
	REPOSITORY=${CI_REPOSITORY_URL}
	BRANCH=${CI_COMMIT_REF_NAME}
fi

if [[ ${#USERNAME} -gt 0 ]] && [[ ${#PASSWORD} -gt 0 ]]; then
	PROTOCOL=$(python -c "print('${REPOSITORY}'.split('://')[0])")
	REPOSITORY="${PROTOCOL}://$USERNAME:$PASSWORD@$(python -c "print('${REPOSITORY}'.split('://')[1].split('@')[1])")"
fi

for IDX in {0...10}; do
	RUN=0

	STATUS=$(./Tools/Utilities/wercker.sh status --token ${WERCKER} --repo ${REPO})
	CODE=0

	if [ $STATUS = 'passed' ] || [ $STATUS = 'finished' ]; then
		RUN=1
		START="HOOK"
		STOP="NOTIFY"
		HOOK="export JOB=build; apt install -y qemu; echo \\\"$REPOSITORY $BRANCH\\\" >> ./repo.list"
		NOTIFY="../Tools/Utilities/wercker.sh env del --name $START --token ${WERCKER} --repo ${REPO}; ../Tools/Utilities/wercker.sh env del --name $STOP --token ${WERCKER} --repo ${REPO}"

		if [ -d /tmp/jobs/$(date +%j) ]; then			
			cat > /tmp/jobs/$(date +%j)/${CI_CONCURRENT_ID}_${CI_CONCURRENT_PROJECT_ID} << EOF
./Tools/Utilities/wercker.sh env del --name "$START" --token ${WERCKER} --repo ${REPO}
./Tools/Utilities/wercker.sh env del --name "$STOP" --token ${WERCKER} --repo ${REPO}
EOF
		fi

		if ! ./Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
			./Tools/Utilities/wercker.sh env del --name "$START" --token ${WERCKER} --repo ${REPO}

			if ! ./Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
				exit -1
			fi
		fi
			
		if ! ./Tools/Utilities/wercker.sh env add --name "$STOP" --value "$NOTIFY" --token ${WERCKER} --repo ${REPO}; then
			./Tools/Utilities/wercker.sh env del --name "$STOP" --token ${WERCKER} --repo ${REPO}

			if ! ./Tools/Utilities/wercker.sh env add --name "$START" --value "$HOOK" --token ${WERCKER} --repo ${REPO}; then
				exit -1
			fi
		fi

		if ! ./Tools/Utilities/wercker.sh restart --token ${WERCKER} --repo ${REPO}; then
			CODE=-1
		fi

		./Tools/Utilities/wercker.sh env del --name "$START" --token ${WERCKER} --repo ${REPO}
		./Tools/Utilities/wercker.sh env del --name "$STOP" --token ${WERCKER} --repo ${REPO}
		if [ $CODE != 0 ]; then
			exit $CODE
		else
			break
		fi
	else
		sleep 600
	fi
done

if [[ $RUN -eq 0 ]]; then
	echo "[  ERROR  ]: Node so busy to perform your task, please try again later or expand the range"
	exit -2
fi
