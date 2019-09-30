#!/bin/bash
# - File: libs/traditional.sh
# - Description: since we are working on traditional way we must prepare our
# system according to the list of project we would like to build now

FAIL=()
METHOD=$1

if ! echo "$2" | grep ".reproduce.d" >& /dev/null; then
	if [ $1 = "inject" ] || [ -f $2 ] || [ ! -d $PIPELINE/Libraries ]; then
		if [ $(basename $0) = 'Environment.sh' ]; then
			PIPELINE="$(dirname $0)"
		else
			PIPELINE="$(dirname $0)/../"
		fi
	else
		PIPELINE=$2
	fi

	source $PIPELINE/Libraries/Logcat.sh
	source $PIPELINE/Libraries/Package.sh
fi

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
	if ! echo "$2" | grep ".reproduce.d" >& /dev/null; then
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
fi

REPO=$3
BRANCH=$4

# @NOTE: otherwide, we should the current environment since the branch maybe
# merged with the master
if [ "$METHOD" = "reproduce" ]; then
	if ! echo "$2" | grep ".reproduce.d" >& /dev/null; then
		if [ -e  $PIPELINE/Environment.sh ]; then
			rm -fr  $PIPELINE/Environment.sh
		fi
		
		# @NOTE: create a shortcut to connect directly with this environment
		ln -s $0 $PIPELINE/Environment.sh

		# @NOTE: okey do reproducing now since we have enough information to do right now
		$PIPELINE/Reproduce.sh
		exit $?
	fi
elif [ "$METHOD" = "prepare" ]; then
	git submodule update --init --recursive

	WORKSPACE=$2
	BASE="$(detect_libbase $WORKSPACE)"
	mkdir -p "$WORKSPACE/build"

	# @NOTE: if we found ./Tests/Pipeline/prepare.sh, it can prove
	# that we are using Eevee as the tool to deploy this Repo
	if [ -e "$BASE/Tests/Pipeline/Prepare.sh" ]; then
		$BASE/Tests/Pipeline/Prepare.sh

		if [ $? != 0 ]; then
			error "Fail repo $REPO/$BRANCH"
		else
			if [ $BASE = $WORKSPACE ]; then
				$WORKSPACE/Tests/Pipeline/Build.sh Base 2
			else
				$WORKSPACE/Tests/Pipeline/Build.sh $(basename $WORKSPACE 2)
			fi

			if [ $? != 0 ]; then
				error "Fail repo $REPO/$BRANCH"
			fi
		fi
	else
		error "we can't support this kind of project for reproducing automatically"
	fi
elif [ "$METHOD" = "test" ]; then
	PIPELINE=$(dirname $0)

	if [ -d ./build ]; then
		cd ./build

		for SPEC in ./*; do
			$PIPELINE/../../Tools/Utilities/ctest.sh $(basename $SPEC) &> $3
			CODE=$?

			if [[ $CODE -ne 0 ]]; then
				break
			fi
		done
	elif [ -f .workspace ]; then
		$ROOT/Tools/Builder/execute
		CODE=$?
	else
		exit -1
	fi

	exit $CODE
elif [ "$METHOD" = "inject" ]; then
	SCREEN=$2

	if [ ! -d $PIPELINE/Plugins ]; then
		exit 0
	elif [ ! -d $PIPELINE/Plugins/Traditional ]; then
		exit 0
	fi

	for PLUGIN in $PIPELINE/Plugins/Traditional/*.sh; do
		if [ ! -x $PLUGIN ]; then
			continue
		elif ! $PLUGIN reset; then
			continue
		fi

		cat > /tmp/$(basename $PLUGIN).conf << EOF
logfile /tmp/$(basename $SCRIPT).log
logfile flush 1
log on
logtstamp after 1
	logtstamp string \"[ %t: %Y-%m-%d %c:%s ]\012\"
logtstamp on
EOF

		screen -c /tmp/$(basename $PLUGIN).conf -L -S $SCREEN -md $PLUGIN
	done
elif [ "$METHOD" = "report" ]; then
	BASE="$(detect_libbase $(pwd))"

	if [ -d ./build ]; then
		cd ./build
		
		if [ -d ./Coverage ]; then
			$BASE/Tools/Utilities/coverage.sh . >& /dev/null
		fi
	fi

	if [ ! -d $PIPELINE/Plugins ]; then
		exit 0
	elif [ ! -d $PIPELINE/Plugins/Traditional ]; then
		exit 0
	fi

	for PLUGIN in $PIPELINE/Plugins/Traditional/*.sh; do
		if [ ! -x $PLUGIN ]; then
			continue
		else
			info "this is the log from plugin $(basename $PLUGIN):"
			$PLUGIN "report"
			echo ""
			echo ""
		fi
	done
elif [ "$METHOD" = "build" ]; then
	if [[ -f './repo.list' ]]; then
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
					echo "$PROJECT" > ./fail
				fi
			done
		else
			if ! process; then
				echo "$PROJECT" > /tmp/fail
			fi
		fi

		# @NOTE: back to the root before jumping to another Repos
		cd $ROOT
		rm -fr $WORKSPACE
	done

	if [ -f /tmp/fail ]; then
		rm -fr /tmp/fail
		exit -1
	else
		exit 0
	fi
fi
