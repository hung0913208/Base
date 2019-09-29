#!/bin/bash
# - File: reproduce.sh
# - Description: This bash script will be used to provide steps to
# reproduce any issue from scratch

LIBRARIES="$(dirname "$0")"
source $LIBRARIES/Logcat.sh
source $LIBRARIES/Package.sh

SCRIPT="$(basename "$0")"
ISSUE=$2
ROOT=$3

function clean() {
	if [ "$1" = "inject" ]; then
		screen -ls "reproduce" | grep -E '\s+[0-9]+\.' | awk -F ' ' '{print $1}' | while read s; do screen -XS $s quit; done

		cat > /tmp/Inject.conf << EOF
logfile /tmp/Inject.log
logfile flush 1
log on
logtstamp after 1
	logtstamp string \"[ %t: %Y-%m-%d %c:%s ]\012\"
logtstamp on
EOF
	fi
}

if [ "$1" = "clone" ]; then
	COMMIT=$7
	PATCH=$8
	REPO=$ROOT
	ROOT=$4
	SPEC=$5
	AUTH=$6

	if [ ! -d "$ROOT/.reproduce.d" ]; then
		mkdir -p "$ROOT/.reproduce.d"
	fi

	git clone "$REPO" "$ROOT/.reproduce.d/$ISSUE"

	if [[ ${#AUTH} -gt 0 ]]; then
		CURRENT=$(pwd)
		if [[ ${#COMMIT} -eq 0 ]]; then
			COMMIT=$(git rev-parse HEAD)
		fi

		cd "$ROOT/.reproduce.d/$ISSUE"
		GERRIT=$(python -c """
uri = '$(git remote get-url --all origin)'.split('/')[2]
gerrit = uri.split('@')[-1].split(':')[0]

print(gerrit)
""")

		QUERY="https://$GERRIT/a/changes/?q=is:open+owner:self&o=CURRENT_REVISION&o=CURRENT_COMMIT"
		RESP=$(curl -s --request GET -u $AUTH "$QUERY" | cut -d "'" -f 2)

		if [[ ${#RESP} -eq 0 ]]; then
			cd "$CURRENT" || error "can't cd $CURRENT"
			exit -1
		fi
		
		REVISIONs=($(echo $RESP | python -c """
import json
import sys

try:
	for resp in json.load(sys.stdin):
		if resp['branch'].encode('utf-8') != '$SPEC':
			continue

		for commit in resp['revisions']:
			revision = resp['revisions'][commit]

			for parent in revision['commit']['parents']:
				if parent['commit'].encode('utf-8') == '$COMMIT':
					print(revision['ref'])
					break
except Exception as error:
	print(error)
	sys.exit(-1)
"""))

		if [ $? != 0 ]; then
			cd "$CURRENT" || error "can't cd $CURRENT"
			exit -1
		fi

		DONE=-1
		for REVIS in "${REVISIONs[@]}"; do
			if [[ ${#PATCH} -eq 0 ]]; then
				info "The patch-set $REVIS base on $COMMIT"

				# @NOTE: checkout the latest patch-set for testing only
				git fetch "$(git remote get-url --all origin)" "$REVIS" >& /dev/null
				git checkout FETCH_HEAD >& /dev/null
				DONE=$?
			elif [[ $(python -c "print('$REVIS'.split('/')[-1])") -eq $PATCH ]]; then
				info "The patch-set $REVIS base on $COMMIT"

				# @NOTE: checkout the latest patch-set for testing only
				git fetch "$(git remote get-url --all origin)" "$REVIS" >& /dev/null
				git checkout FETCH_HEAD >& /dev/null
				DONE=$?
			fi
		done

		cd "$CURRENT" || error "can't cd $CURRENT"
		if [[ $DONE -ne 0 ]]; then
			error "Can't find the patch-set $PATCH"
		fi
	elif [ $# -gt 3 ]; then
		cd "$ROOT/.reproduce.d/$ISSUE" || error "can't cd $ROOT/.reproduce.d/$ISSUE"

		if [[ $(grep "$SPEC" <<< $(git branch -a)) ]]; then
			git checkout -b $SPEC "origin/$SPEC"
		else
			git fetch --all
			git pull --all
			git checkout -b $(git name-rev --name-only $SPEC) $SPEC
		fi
		CODE=$?

		cd "$CURRENT" || error "can't cd $CURRENT"
		if [ "$CODE" != 0 ]; then
			error "fail to clone $ISSUE with spec $SPEC"
		fi
	fi

	if [ $? != 0 ]; then
		error "fail to clone $ISSUE"
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh" ]; then
		info "found script Fetch.sh, halt instead of invoking Create.sh"
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Create.sh" ]; then
		"$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Create.sh" "reproduce" "$ROOT/.reproduce.d/$ISSUE"

		if [ $? != 0 ]; then
			error "can't perform script create.sh to generate reproducing scripts"
		fi
	fi
elif [ "$1" = "prepare" ]; then
	CURRENT=$(pwd)

	if [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh" ] || [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh" ]; then
		if [ -d "$ROOT/.reproduce.d/$ISSUE/.requirement.d" ]; then
			cat "$ROOT/.reproduce.d/$ISSUE/.requirement.d/$SYS" | while read PACKAGE; do
				install_package $PACKAGE

				if [ $? != 0 ]; then
					warning "problem with install $PACKAGE"
				fi
			done
		fi

		if [ -f "$ROOT/.reproduce.d/$ISSUE/.requirement" ]; then
			cat "$ROOT/.reproduce.d/$ISSUE/.requirement" | while read PACKAGE; do
				install_package $PACKAGE

				if [ $? != 0 ]; then
					warning "problem with install $PACKAGE"
				fi
			done
		fi

		if [ -d "$ROOT/.reproduce.d/$ISSUE/.recompile.d" ]; then
			cat "$ROOT/.reproduce.d/$ISSUE/.recompile.d/$SYS" | while read DEFINE; do
				if [[ ${#DEFINE} -gt 0 ]]; then
					SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
					REPO=${SPLITED[0]}
					BACKGROUND=${SPLITED[1]}

					info "recompile $REPO now, $BACKGROUND background"
					cd "$ROOT/.reproduce.d/$ISSUE" || error "can't cd to $ROOT/.reproduce.d/$ISSUE"


					if [ "$BACKGROUND" == "show" ]; then
						$LIBRARIES/Install.sh $DEFINE
					else
						$LIBRARIES/Install.sh $DEFINE &> /dev/null
					fi
					CODE=$?

					cd $CURRENT
					if [ $CODE != 0 ]; then
						error "fail recompile $DEFINE"
					fi
				fi
			done
		fi

		if [ -f "$ROOT/.reproduce.d/$ISSUE/.recompile" ]; then
			cat "$ROOT/.reproduce.d/$ISSUE/.recompile" | while read DEFINE; do
				if [[ ${#DEFINE} -gt 0 ]]; then
					SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
					REPO=${SPLITED[0]}
					BACKGROUND=${SPLITED[1]}

					info "recompile $REPO now, $BACKGROUND background"
					cd "$ROOT/.reproduce.d/$ISSUE" || error "can't cd to $ROOT/.reproduce.d/$ISSUE"

					if [ $BACKGROUND == "show" ]; then
						$LIBRARIES/Install.sh $DEFINE
					else
						$LIBRARIES/Install.sh $DEFINE &> /dev/null
					fi
					CODE=$?

					cd $CURRENT
					if [ $CODE != 0 ]; then
						error "fail recompile $DEFINE"
					fi
				fi
			done
		fi
	elif [ -e "$LIBRARIES/../Environment.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		"$LIBRARIES/../Environment.sh" "prepare" "$ROOT/.reproduce.d/$ISSUE"
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/fetch.sh"
		fi
	else
		echo "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/"
		ls "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/"
		error "not found an approviated environment for reproducing"
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/fetch.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/fetch.sh "$ROOT/.reproduce.d/$ISSUE"
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/fetch.sh"
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh "$ROOT/.reproduce.d/$ISSUE"
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Fetch.sh"
		fi
	fi

	if [ -f "$ROOT/.reproduce.d/$ISSUE/install.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/install.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/install.sh"
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Install.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Install.sh

		if [ $? != 0 ]; then
			error "fail when run $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Install.sh"
		fi
	fi
elif [ "$1" = "reproduce" ]; then
	if [ -f "$ROOT/.reproduce.d/$ISSUE/test.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/test.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit 0
		else
			exit 1 # <--- try to redo again and again
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Test.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Test.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		else
			exit 1 # <--- try to redo again and again
		fi
	elif [ -f "$LIBRARIES/../Environment.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		"$LIBRARIES/../Environment.sh" "test" "$ROOT/.reproduce.d/$ISSUE"
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		else
			exit 1 # <--- try to redo again and again
		fi
	else
		exit -1
	fi
	exit 0
elif [ "$1" = "verify" ]; then
	LOGS=$4
	CODE=$5

	if [ "$CODE" != 0 ] && [ "$CODE" != 1 ]; then
		exit $CODE
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/verify.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/verify.sh $LOGS/$ISSUE
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Verify.sh" ]; then
		CURRENT=$(pwd)

		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Verify.sh $LOGS
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	fi
	exit 0
elif [ "$1" = "inject" ]; then
	CURRENT=$(pwd)

	if [ $4 != 0 ] && [ $4 != 1 ]; then
		exit $4
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/inject.sh" ]; then
		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		clean "inject"
		screen -c /tmp/Inject.conf -L -S "reproduce" -md $ROOT/.reproduce.d/$ISSUE/inject.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject.sh" ]; then
		cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"
		clean "inject"
		screen -c /tmp/Inject.conf -L -S "reproduce" -md $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject.sh
		CODE=$?

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	elif [ -d "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject" ]; then
		clean "inject"

		for SCRIPT in $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Inject/*.sh; do
			cat > /tmp/$(basename $SCRIPT).conf << EOF
logfile /tmp/$(basename $SCRIPT).log
logfile flush 1
log on
logtstamp after 1
	logtstamp string \"[ %t: %Y-%m-%d %c:%s ]\012\"
logtstamp on
EOF
			screen -c /tmp/$(basename $SCRIPT).conf -L -S "reproduce" -md "$SCRIPT"
			CODE=$?

			if [ $CODE != 0 ]; then
				break
			fi
		done

		cd $CURRENT || error "can't cd to $CURRENT"
		if [ $CODE != 0 ]; then
			exit -1
		fi
	elif [ -f "$LIBRARIES/../Environment.sh" ]; then
		clean "inject"

		$LIBRARIES/../Environment.sh "inject" "reproduce"
	fi
	exit 0
elif [ "$1" = "report" ]; then
	CURRENT=$(pwd)
	cd $ROOT/.reproduce.d/$ISSUE || error "can't cd to $ROOT/.reproduce.d/$ISSUE"

	if [ -f "$ROOT/.reproduce.d/$ISSUE/report.sh" ]; then
		"$ROOT/.reproduce.d/$ISSUE/report.sh" $3
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Report.sh" ]; then
		"$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Report.sh" $3
	elif [ -f "$ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Report" ]; then
		for REPORT in $ROOT/.reproduce.d/$ISSUE/Tests/Pipeline/Report/*.sh; do
			$REPORT $4
		done
	elif [ -f "$LIBRARIES/../Environment.sh" ]; then
		"$LIBRARIES/../Environment.sh" report $4
	fi

	cd $CURRENT || error "can't cd to $CURRENT"
	exit 0
else
	error "no support $1"
fi

info "Congratulation, you have done step $1"

