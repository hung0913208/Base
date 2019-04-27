#!/bin/bash
# - File: libs/traditional.sh
# - Description: since we are working on traditional way we must prepare our
# system according to the list of project we would like to build now

PIPELINE=$1
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"
REPO=$2

# @NOTE: fetch the list project we would like to build from remote server
if [ ! -f './repo.list' ]; then
	curl -k --insecure $REPO -o './repo.list' >> /dev/null
	if [ $? != 0 ]; then
		error "Can't fetch list of project from $REPO"
	fi
fi

# @NOTE: perform testing on wire range
cat './repo.list' | while read DEFINE; do
	SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
	REPO=${SPLITED[0]}
	BRANCH=${SPLITED[1]}
	PROJECT=$(basename $REPO)

	# @NOTE: clone this Repo, including its submodules
	git clone --single-branch --branch $BRANCH $REPO $PROJECT
	if [ $? != 0 ]; then
		FAIL+=$PROJECT
	else
		ROOT=$(pwd)
		WORKSPACE="$ROOT/$PROJECT"

		# @NOTE: everything was okey from now, run CI steps
		cd $WORKSPACE
		git submodule update --init --recursive

		mkdir -p "./build"

		# @NOTE: if we found ./Tests/Pipeline/prepare.sh, it can prove
		# that we are using Eevee as the tool to deploy this Repo
		if [ -e "./Tests/Pipeline/Prepare.sh" ]; then
			./Tests/Pipeline/Prepare.sh

			if [ $? != 0 ]; then
				warning "Fail repo $REPO/$BRANCH"
			else
				./Tests/Pipeline/Build.sh

				if [ $? != 0 ]; then
					warning "Fail repo $REPO/$BRANCH"
				fi
			fi
		else
			warning "repo $REPO don't support usual CI method"
		fi

		# @NOTE: back to the root before jumping to another Repos
		cd $ROOT
		rm -fr $WORKSPACE
	fi
done
