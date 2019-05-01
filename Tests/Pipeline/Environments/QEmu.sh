#!/bin/bash

REPO=$3
METHOD=$1
BRANCH=$4
PIPELINE=$2

source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

git branch | egrep 'Pipeline/QEmu$' >& /dev/null
if [ $? != 0 ]; then
	CURRENT=$(pwd)

	cd $PIPELINE || error "can't cd to $PIPELINE"
	git branch -a | egrep 'remotes/origin/Pipeline/QEmu$' >& /dev/null

	if [ $? = 0 ]; then
		# @NOTE: by default we will fetch the maste branch of this environment
		# to use the best candidate of the branch

		git fetch >& /dev/null
		git checkout -b "Pipeline/QEmu" "origin/Pipeline/QEmu"
		if [ $? = 0 ]; then
			"$0" $METHOD $PIPELINE $REPO $BRANCH
		fi

		exit $?
	fi

	cd $CURRENT || error "can't cd to $CURRENT"
fi

# @NOTE: otherwide, we should the current environment since the branch maybe
# merged with the master
if [ "$METHOD" == "reproduce" ]; then
	$PIPELINE/Reproduce.sh $REPO
	exit $?
elif [[ $# -gt 3 ]]; then
	BRANCH=$4
	info "import '$REPO $BRANCH' >> ./repo.list"

	echo "$REPO $BRANCH" >> ./repo.list
else
	# @NOTE: fetch the list project we would like to build from remote server
	if [ ! -f './repo.list' ]; then
		curl -k --insecure $REPO -o './repo.list' >> /dev/null
		if [ $? != 0 ]; then
			error "Can't fetch list of project from $REPO"
		fi
	fi
fi
