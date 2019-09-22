#!/bin/bash
# - File: libs/traditional.sh
# - Description: since we are working on traditional way we must prepare our
# system according to the list of project we would like to build now

FAIL=()
REPO=$3
METHOD=$1
BRANCH=$4
PIPELINE=$2

source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

function detect_libbase() {
	PROJECT=$1

	if [[ -d "${PROJECT}/Eden" ]]; then
		echo "${PROJECT}/Eden"
	elif [[ -d "${PROJECT}/Base" ]]; then
		echo "${PROJECT}/Base"
	elif [[ -d "${PROJECT}/base" ]]; then
		echo "${PROJECT}/base"
	elif [[ -d "${PROJECT}/LibBase" ]]; then
		echo "${PROJECT}/LibBase"
	elif [[ -d "${PROJECT}/Eevee" ]]; then
		echo "${PROJECT}/Eevee"
	else
		echo "$PROJECT"
	fi
}

git branch | egrep 'Pipeline/Traditional$'
if [ $? != 0 ]; then
	CURRENT=$(pwd)

	cd $PIPELINE || error "can't cd to $PIPELINE"
	git branch -a | egrep 'remotes/origin/Pipeline/Traditional$'

	if [ $? = 0 ]; then
		# @NOTE: by default we will fetch the maste branch of this environment
		# to use the best candidate of the branch

		git fetch >& /dev/null
		git checkout -b "Pipeline/Traditional" "origin/Pipeline/Traditional"
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
	$PIPELINE/Reproduce.sh
	exit $?
elif [[ -f './repo.list' ]]; then
	info "use default ./repo.list"
elif [[ $# -gt 3 ]]; then
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

function process() {
	CODE=0
	git submodule update --init --recursive

	BASE="$(detect_libbase $WORKSPACE)"
	mkdir -p "$WORKSPACE/build"

	# @TODO: since we will use gerrit to manage revision so i should
	# consider a new way to report the test result to gerrit too

	# @NOTE: if we found ./Tests/Pipeline/prepare.sh, it can prove
	# that we are using Eevee as the tool to deploy this Repo
	if [ -e "$BASE/Tests/Pipeline/Prepare.sh" ]; then
		$BASE/Tests/Pipeline/Prepare.sh

		if [ $? != 0 ]; then
			warning "Fail repo $REPO/$BRANCH"
			CODE=1
		else
			$WORKSPACE/Tests/Pipeline/Build.sh

			if [ $? != 0 ]; then
				warning "Fail repo $REPO/$BRANCH"
				CODE=1
			fi
		fi
	else
		warning "repo $REPO don't support usual CI method"
		CODE=1
	fi

	return $CODE
}

# @NOTE: perform testing on wire range
cat './repo.list' | while read DEFINE; do
	SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
	REPO=${SPLITED[0]}
	BRANCH=${SPLITED[1]}
	PROJECT=$(basename $REPO)
	PROJECT=${PROJECT%.*}

	if [[ ${#SPLITED[@]} -gt 2 ]]; then
		AUTH=${SPLITED[2]}
	fi

	# @NOTE: clone this Repo, including its submodules
	git clone --branch $BRANCH $REPO $PROJECT

	if [ $? != 0 ]; then
		FAIL=(${FAIL[@]} $PROJECT)
		continue
	fi

	ROOT=$(pwd)
	WORKSPACE="$ROOT/$PROJECT"

	# @NOTE: everything was okey from now, run CI steps
	cd $WORKSPACE

	# @NOTE: detect an authenticate key of gerrit so we will use it
	# to detect which patch set should be used to build
	if [[ ${#AUTH} -gt 0 ]]; then
		if [[ ${#SPLITED[@]} -gt 3 ]]; then
			COMMIT=${SPLITED[3]}
		else
			COMMIT=$(git rev-parse HEAD)
		fi

		GERRIT=$(python -c """
uri = '$(git remote get-url --all origin)'.split('/')[2]
gerrit = uri.split('@')[-1].split(':')[0]

print(gerrit)
""")

		QUERY="https://$GERRIT/a/changes/?q=is:open+owner:self&o=CURRENT_REVISION&o=CURRENT_COMMIT"
		RESP=$(curl -s --request GET -u $AUTH "$QUERY" | cut -d "'" -f 2)

		if [[ ${#RESP} -eq 0 ]]; then
			FAIL=(${FAIL[@]} $PROJECT)
			continue
		fi

		REVISIONs=($(echo $RESP | python -c """
import json
import sys

try:
	for  resp in json.load(sys.stdin):
		if resp['branch'] != '$BRANCH':
			continue

		for commit in resp['revisions']:
			revision = resp['revisions'][commit]

			for parent in revision['commit']['parents']:
				if parent['commit'] == '$COMMIT':
					print(revision['ref'])
					break
except Exception as error:
	print(error)
	sys.exit(-1)
"""))

		if [ $? != 0 ]; then
			FAIL=(${FAIL[@]} $PROJECT)
			continue
		fi

		for REVIS in "${REVISIONs[@]}"; do
			info "The patch-set $REVIS base on $COMMIT"

			# @NOTE: checkout the latest patch-set for testing only
			git fetch "$(git remote get-url --all origin)" "$REVIS"
			git checkout FETCH_HEAD

			if ! process; then
				FAIL+=$PROJECT
				break
			fi
		done
	else
		if ! process; then
			FAIL=(${FAIL[@]} $PROJECT)
			break
		fi
	fi


	# @NOTE: back to the root before jumping to another Repos
	cd $ROOT
	rm -fr $WORKSPACE
done

if [[ ${#FAIL[@]} -gt 0 ]]; then
	exit -1
else
	exit 0
fi
